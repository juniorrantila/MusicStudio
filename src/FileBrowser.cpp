#include "./FileBrowser.h"
#include "./EventLoop.h"
#include "./StatusBar.h"
#include "./Style.h"

#include "./PathEvent.h"

#include <UI/FreeGlyph.h>
#include <UI/SimpleRenderer.h>
#include <UI/UI.h>

#include <Rexim/Util.h>
#include <Rexim/StringView.h>

#include <Ty/StringBuffer.h>
#include <Ty/Optional.h>

static int file_cmp(const void *ap, const void *bp)
{
    c_string a = *(c_string*)ap;
    c_string b = *(c_string*)bp;
    return strcmp(a, b);
}

ErrorOr<FileBrowser> FileBrowser::create(StringView path)
{
    auto browser = FileBrowser();
    TRY(browser.open_dir(path));
    return browser;
}

ErrorOr<void> FileBrowser::open_dir(StringView path)
{
    auto path_buf = TRY(StringBuffer::create_fill(path, "\0"sv));

    m_files.count = 0;
    m_cursor = 0;

    if (auto err = read_entire_dir(path_buf.data(), &m_files); err != 0)
        return Error::from_errno(err);
    qsort(m_files.items, m_files.count, sizeof(*m_files.items), file_cmp);

    m_dir_path.count = 0;
    sb_append_cstr(&m_dir_path, path_buf.data());
    sb_append_null(&m_dir_path);

    return {};
}

#define PATH_SEP "/"
#define PATH_EMPTY ""
#define PATH_DOT "."
#define PATH_DOTDOT ".."

typedef struct {
    String_View* items;
    usize count;
    usize capacity;
} Comps;

void normpath(String_View path, String_Builder *result)
{
    usize original_sb_size = result->count;

    if (path.count == 0) {
        sb_append_cstr(result, PATH_DOT);
        return;
    }

    int initial_slashes = 0;
    while (path.count > 0 && *path.data == *PATH_SEP) {
        initial_slashes += 1;
        sv_chop_left(&path, 1);
    }
    if (initial_slashes > 2) {
        initial_slashes = 1;
    }

    Comps new_comps = {};

    while (path.count > 0) {
        String_View comp = sv_chop_by_delim(&path, '/');
        if (comp.count == 0 || sv_eq(comp, SV(PATH_DOT))) {
            continue;
        }
        if (!sv_eq(comp, SV(PATH_DOTDOT))) {
            da_append(&new_comps, comp);
            continue;
        }
        if (initial_slashes == 0 && new_comps.count == 0) {
            da_append(&new_comps, comp);
            continue;
        }
        if (new_comps.count > 0 && sv_eq(da_last(&new_comps), SV(PATH_DOTDOT))) {
            da_append(&new_comps, comp);
            continue;
        }
        if (new_comps.count > 0) {
            new_comps.count -= 1;
            continue;
        }
    }

    for (int i = 0; i < initial_slashes; ++i) {
        sb_append_cstr(result, PATH_SEP);
    }

    for (usize i = 0; i < new_comps.count; ++i) {
        if (i > 0) sb_append_cstr(result, PATH_SEP);
        sb_append_buf(result, new_comps.items[i].data, new_comps.items[i].count);
    }

    if (original_sb_size == result->count) {
        sb_append_cstr(result, PATH_DOT);
    }

    free(new_comps.items);
}

ErrorOr<void> FileBrowser::change_dir()
{
    assert(m_dir_path.count > 0 && "You need to call fb_open_dir() before fb_change_dir()");
    assert(m_dir_path.items[m_dir_path.count - 1] == '\0');

    if (m_cursor >= m_files.count) return {};

    c_string dir_name = m_files.items[m_cursor].name;

    String_Builder new_path = {};
    da_append_many(&new_path, m_dir_path.items, m_dir_path.count);

    new_path.count -= 1;

    // TODO: fb->dir_path grows indefinitely if we hit the root
    sb_append_cstr(&new_path, "/");
    sb_append_cstr(&new_path, dir_name);

    String_Builder result = {};
    normpath(sb_to_sv(new_path), &result);
    da_move(&new_path, result);
    sb_append_null(&new_path);

    Files new_files = {};
    Errno err = read_entire_dir(new_path.items, &new_files);
    if (err != 0)
        return Error::from_errno(err);

    da_move(&m_files, new_files);
    da_move(&m_dir_path, new_path);
    m_cursor = 0;
    printf("Changed dir to %s\n", m_dir_path.items);

    qsort(m_files.items, m_files.count, sizeof(*m_files.items), file_cmp);

    return {};
}

Optional<StringView> FileBrowser::current_file()
{
    assert(m_dir_path.count > 0 && "You need to call fb_open_dir() before fb_file_path()");
    assert(m_dir_path.items[m_dir_path.count - 1] == '\0');

    if (m_cursor >= m_files.count) return {};

    m_file_path.count = 0;
    sb_append_buf(&m_file_path, m_dir_path.items, m_dir_path.count - 1);
    sb_append_buf(&m_file_path, "/", 1);
    sb_append_cstr(&m_file_path, m_files.items[m_cursor].name);
    sb_append_null(&m_file_path);

    return StringView::from_c_string(m_file_path.items);
}

void FileBrowser::render(UI::UI& ui, EventLoop& event_loop, Vec4f box)
{
    f32 border_size = 2.0f;

    auto font_size = Style::the().file_browser_font_size();
    MUST(ui.set_font_size(font_size));

    ui.outline_rect({
        .box = box,
        .outline_size = border_size,
        .fill_color = Style::the().file_browser_color(),
        .right_color = Style::the().file_browser_border_color(),
    });

    auto indent_size = Style::the().file_browser_indent_width();
    ui.fill_rect(vec4fv(
            vec2f(0.0f, 0.0f),
            vec2f(indent_size, box.height)
        ),
        Style::the().file_browser_indent_color()
    );

    for (usize row = 0; row < m_files.count; ++row) {
        const Vec2f pos = vec2f(indent_size + 2.0f, box.height - ((f32)row + 1) * font_size.y);
        StringView file_name = StringView::from_c_string(m_files.items[row].name);

        auto text_size = ui.measure_text(file_name);
        ui.text(vec4fv(pos, box.size() - pos + box.start_point()), file_name, Style::the().text_color());

        if (text_size.x < box.width) {
            if (m_files.items[row].type == FT_DIRECTORY) {
                auto slash_pos = pos + text_size;
                auto text_box = vec4fv(slash_pos, box.size() - slash_pos);
                ui.text(text_box, "/"sv, Style::the().text_alternate_color());
            }
        }

        auto box = vec4fv(
            pos - vec2f(0.0f, 3.0f),
            vec2f(200.0f - indent_size - 3.0f * 2.0f, font_size.y)
        );
        auto button = ui.button(box);
        switch (button.state()) {
        case UI::ButtonState::Action:
            ui.fill_rect(box, vec4f(1.0f, 0.0f, 0.0f, 0.50));
            if (auto file_path = current_file()) {
                MUST(event_loop.dispatch_event(ChangePathEvent {
                    .file_path_buf = MUST(StringBuffer::create_fill(file_path.value(), "\0"sv)),
                }));
            }
            break;
        case UI::ButtonState::Pressed:
            ui.fill_rect(box, vec4f(0.2f, 0.5f, 0.5f, 0.25));
            break;
        case UI::ButtonState::Hovered:
            if (auto file_path = current_file()) {
                if (file_path != m_hovered_file) {
                    m_hovered_file = file_path.value();
                    StatusBar::the().set_text(StatusKind::Info, "%.*s", file_path->size(), file_path->data());
                }
            }
            ui.fill_rect(box, vec4f(1.0f, 1.0f, 1.0f, 0.25));
            m_cursor = row;
            break;
        case UI::ButtonState::None:
            break;
        }
    }
}

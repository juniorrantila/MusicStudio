#include "./FileBrowser.h"
#include "./EventLoop.h"
#include "./StatusBar.h"
#include "./Style.h"

#include "./PathEvent.h"
#include "Rexim/LA.h"

#include <UIView/View.h>
#include <UIView/List.h>
#include <UIView/Text.h>
#include <UIView/Button.h>
#include <UIView/Box.h>

#include <UI/FreeGlyph.h>
#include <UI/SimpleRenderer.h>
#include <UI/UI.h>

#include <Rexim/Util.h>
#include <Rexim/StringView.h>

#include <Ty/StringBuffer.h>
#include <Ty/Optional.h>

using UIView::list;
using UIView::text;
using UIView::button;
using UIView::box;

static int file_cmp(const void *ap, const void *bp)
{
    c_string a = *(c_string const*)ap;
    c_string b = *(c_string const*)bp;
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

static void normpath(String_View path, String_Builder *result)
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

UIView::ViewBase* FileBrowser::view()
{
    // FIXME
    // ui.outline_rect({
    //     .box = box,
    //     .outline_size = 2.0f,
    //     .fill_color = Style::the().file_browser_color(),
    //     .right_color = Style::the().file_browser_border_color(),
    // });
    auto files = View(m_files.items, m_files.count);
    return list(files, [&, this](File file, usize index) {
        return button(" %s%s", file.name, file.type == FT_DIRECTORY ? "/" : "")
            ->set_text_color(Style::the().text_color())
            ->set_font_size(Style::the().file_browser_font_size().y)
            ->set_background_color(Style::the().file_browser_color())
            ->hover_style([](UIView::Button* self) {
                self->set_background_color(vec4f(1.0f, 1.0f, 1.0f, 0.25f));
            })
            ->action_style([](UIView::Button* self) {
                self->set_background_color(vec4f(1.0f, 0.0f, 0.0f, 0.50f));
            })
            ->press_style([](UIView::Button* self) {
                self->set_background_color(vec4f(0.2f, 0.5f, 0.5f, 0.25f));
            })
            ->hover([=, this] {
                if (auto file_path = current_file()) {
                    if (file_path != m_hovered_file) {
                        m_hovered_file = file_path.value();
                        StatusBar::the().set_text(StatusKind::Info, "%.*s", file_path->size(), file_path->data());
                    }
                }
                m_cursor = index;
            })
            ->action([this] {
                if (auto file_path = current_file()) {
                    this->selected_file.update(file_path.value());
                }
            });
    });
}

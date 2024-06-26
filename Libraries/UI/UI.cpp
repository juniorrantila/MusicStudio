#include "./UI.h"
#include "./FreeGlyph.h"
#include "./SimpleRenderer.h"

#include <Ty/Hash.h>

namespace UI {

UI::UI(SimpleRenderer&& simple_renderer)
    : m_renderer(move(simple_renderer))
{
}

Vec2f UI::mouse_pos() const
{
    return m_mouse_pos * m_renderer.resolution();
}

void UI::set_mouse_pos(f32 x, f32 y)
{
    auto titlebar_size = vec2f(0.0f, m_title_bar_height);
    m_mouse_pos = vec2f(x, y) / (m_renderer.resolution() - titlebar_size);
}

void UI::clear(Vec4f color) const
{
    glClearColor(color.x, color.y, color.z, color.w);
    glClear(GL_COLOR_BUFFER_BIT);
}

void UI::begin_frame()
{
    m_uid = 0;
}

void UI::end_frame()
{
    m_scroll_x = 0;
    m_scroll_y = 0;
    m_renderer.flush();
}

Button UI::button(Vec4f box, StringView file, u32 line)
{
    i64 uid = Hash()
        .djbd(m_uid++)
        .djbd(file.data(), file.size())
        .djbd(line)
        .hash();

    bool pressed = false;
    bool hovered = false;
    bool action = false;
    if (mouse_pos().is_inside(box)) {
        hovered = true;
        if (m_active_id == uid) {
            pressed = true;
            if (!m_mouse_left_down) {
                action = true;
                m_active_id = null_action;
            }
        } else if (m_active_id == null_action) {
            if (m_mouse_left_down) {
                pressed = true;
                m_active_id = uid;
            } else {
                pressed = false;
                m_active_id = null_action;
            }
        }
    } else if (m_active_id == uid && !m_mouse_left_down) {
        m_active_id = null_action;
    }

    return {
        .ui = *this,
        .box = box,
        .action = action,
        .pressed = pressed,
        .hovered = hovered,
    };
}

Tab UI::tab(Vec4f box, StringView, u32)
{
    bool hovered = false;
    bool action = false;
    if (m_mouse_pos.x >= box.x && m_mouse_pos.x <= box.x + box.width) {
        if (m_mouse_pos.y >= box.y && m_mouse_pos.y < box.y + box.height) {
            if (m_active_id == -1 ) {
                if (m_mouse_left_down) {
                    action = true;
                }
            }
            hovered = true;
        }
    }

    return {
        .ui = *this,
        .box = box,
        .hovered = hovered,
        .action = action,
    };
}

void UI::fill_rect(Vec4f box, Vec4f color)
{
    m_renderer.set_shader(SHADER_FOR_COLOR);
    m_renderer.solid_rect(
        vec2f(box.x, box.y),
        vec2f(box.width, box.height),
        color
    );
}

void UI::outline_rect(Vec4f box, f32 outline_size, Vec4f fill_color, Vec4f outline_color)
{
    m_renderer.set_shader(SHADER_FOR_COLOR);
    m_renderer.outline_rect(
        vec2f(box.x, box.y), vec2f(box.width, box.height),
        outline_size, fill_color, outline_color);
}

void UI::outline_rect(UI::OutlineRect const& args)
{
    m_renderer.set_shader(SHADER_FOR_COLOR);
    m_renderer.outline_rect({
        .point = vec2f(args.box.x, args.box.y),
        .size = vec2f(args.box.width, args.box.height),
        .outline_size = args.outline_size,
        .fill_color = args.fill_color,
        .left_color = args.left_color,
        .top_color = args.top_color,
        .right_color = args.right_color,
        .bottom_color = args.bottom_color,
    });
}

ErrorOr<void> UI::load_font(Bytes ttf_data, Vec2f font_size)
{
    if (m_font_library == nullptr) {
        if (FT_Init_FreeType(&m_font_library) != FT_Err_Ok) {
            return Error::from_string_literal("Could not initialize FreeType2 library");
        }
    }

    auto error = FT_New_Memory_Face(m_font_library, ttf_data.data(), ttf_data.size(), 0, &m_font_face);
    if (error) {
        return Error::from_string_literal(FT_Error_String(error));
    }

    TRY(set_font_size(font_size));

    return {};
}

ErrorOr<void> UI::set_font_size(Vec2f size)
{
    if (m_current_font_size == size)
        return {};

    auto error = FT_Set_Pixel_Sizes(m_font_face, size.x, size.y);
    if (error) {
        return Error::from_string_literal(FT_Error_String(error));
    }

    if (m_font_atlas.find(size)) {
        m_current_font_size = size;
        atlas().load();
        return {};
    }

    TRY(m_font_atlas.append(size, TRY(FreeGlyphAtlas::create(m_font_face))));
    m_current_font_size = size;

    return {};
}

Vec2f UI::measure_text(StringView text) const
{
    return atlas().measure_line_sized(text);
}

void UI::text(Vec2f pos, StringView content, Vec4f color)
{
    auto size = atlas().measure_line_sized(content);
    text(vec4fv(pos, size + vec2f(0.0f, m_current_font_size.x)), content, color);
}

void UI::text(Vec4f box, StringView text, Vec4f color)
{
    m_renderer.set_shader(SHADER_FOR_TEXT);
    atlas().render_line_sized(&m_renderer, text, box, color);
}

}

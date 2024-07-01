#pragma once
#include "./Forward.h"

#include <Ty/Base.h>
#include <Ty/StringView.h>
#include <Rexim/LA.h>

#include <UI/FreeGlyph.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <Ty/SmallMap.h>
#include <UI/SimpleRenderer.h>

namespace UI {

struct Button;
struct Tab;
struct UI {
    UI(SimpleRenderer&& sr);

    void set_scroll_x(i32 x) { m_scroll_x = x; }
    void set_scroll_y(i32 y) { m_scroll_y = y; }
    void set_mouse_down(bool down) { m_mouse_left_down = down; }

    void clear(Vec4f color) const;

    Button button(Vec4f box, StringView file = __builtin_FILE(), u32 line = __builtin_LINE());
    Tab tab(Vec4f box, StringView file = __builtin_FILE(), u32 line = __builtin_LINE());

    void fill_rect(Vec4f box, Vec4f color);

    struct OutlineRect {
        Vec4f box;
        f32 outline_size;

        Vec4f fill_color;
        Vec4f left_color;
        Vec4f top_color;
        Vec4f right_color;
        Vec4f bottom_color;
    };
    void outline_rect(OutlineRect const&);
    void outline_rect(Vec4f box, f32 outline_size, Vec4f fill_color, Vec4f outline_color);

    void begin_frame();
    void end_frame();

    Vec2f measure_text(StringView text) const;

    void text(Vec2f pos, StringView text, Vec4f color);
    void text(Vec4f box, StringView text, Vec4f color);

    ErrorOr<void> load_font(Bytes ttf_data, Vec2f font_size);
    ErrorOr<void> set_font_size(Vec2f);

    Vec2f mouse_pos() const;
    void set_mouse_pos(f32 x, f32 y);

    void set_title_bar_height(f32 height)
    {
        m_title_bar_height = height;
    }

    void set_resolution(Vec2f resolution)
    {
        m_renderer.set_resolution(resolution);
        m_renderer.set_camera_pos(resolution / 2.0f);
    }

    Vec2f resolution() const { return m_renderer.resolution(); }

private:
    FreeGlyphAtlas const& atlas() const
    {
        auto id = MUST(m_font_atlas.find(m_current_font_size).or_throw([]{
            return Error::from_string_literal("invalid font size");
        }));
        return m_font_atlas[id];
    }

    FreeGlyphAtlas& atlas()
    {
        auto id = MUST(m_font_atlas.find(m_current_font_size).or_throw([]{
            return Error::from_string_literal("invalid font size");
        }));
        return m_font_atlas[id];
    }

    u64 m_uid { 0 };

    using FontSize = Vec2f;

    SimpleRenderer m_renderer;
    FT_Library m_font_library { nullptr };
    FT_Face m_font_face { nullptr };
    SmallMap<FontSize, FreeGlyphAtlas> m_font_atlas {};
    FontSize m_current_font_size { 0.0f, 0.0f };

    static constexpr i64 null_action = -1;
    i64 m_active_id { null_action };

    Vec2f m_mouse_pos { 0.0f, 0.0f };

    i32 m_scroll_x { 0 };
    i32 m_scroll_y { 0 };

    f32 m_title_bar_height { 0.0f };

    bool m_mouse_left_down : 1 { false };
};

enum class ButtonState {
    None,
    Hovered,
    Pressed,
    Action,
};

struct Button {
    UI& ui;
    Vec4f box;

    bool action : 1;
    bool pressed : 1;
    bool hovered : 1;

    ButtonState state() const
    {
        if (action)
            return ButtonState::Action;
        if (pressed)
            return ButtonState::Pressed;
        if (hovered)
            return ButtonState::Hovered;
        return ButtonState::None;
    }

    void fill_rect(Vec4f color) const;
};
inline void Button::fill_rect(Vec4f color) const { ui.fill_rect(box, color); }

struct Tab {
    UI& ui;
    Vec4f box;

    bool hovered : 1;
    bool action : 1;

    void fill_rect(Vec4f color) const;
};
inline void Tab::fill_rect(Vec4f color) const { ui.fill_rect(box, color); }

}

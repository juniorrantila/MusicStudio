#pragma once
#include "./Forward.h"

#include <Ty/Base.h>
#include <Ty/StringView.h>
#include <Rexim/LA.h>

namespace UI {

struct Button;
struct UI {
    UI(SimpleRenderer* sr, FreeGlyphAtlas* atlas);

    UI(UI&& other)
        : m_uid(other.m_uid)
        , m_renderer(other.m_renderer)
        , m_active_id(other.m_active_id)
        , m_last_mouse_pos(other.m_last_mouse_pos)
        , m_mouse_pos(other.m_mouse_pos)
        , m_scroll_x(other.m_scroll_x)
        , m_scroll_y(other.m_scroll_y)
        , m_time(other.m_time)
        , m_mouse_left_down(other.m_mouse_left_down)
    {
        other.invalidate();
    }
    ~UI();

    UI& operator=(UI&& other)
    {
        if (&other == this)
            return *this;
        this->~UI();
        m_uid = other.m_uid;
        m_renderer = other.m_renderer;
        m_active_id = other.m_active_id;
        m_last_mouse_pos = other.m_last_mouse_pos;
        m_mouse_pos = other.m_mouse_pos;
        m_scroll_x = other.m_scroll_x;
        m_scroll_y = other.m_scroll_y;
        m_time = other.m_time;
        m_mouse_left_down = other.m_mouse_left_down;
        other.invalidate();
        return *this;
    }

    void set_scroll_x(i32 x) { m_scroll_x = x; }
    void set_scroll_y(i32 y) { m_scroll_y = y; }
    void set_mouse_down(bool down) { m_mouse_left_down = down; }

    void clear(Vec4f color) const;

    Button button(Vec4f box, c_string file = __builtin_FILE(), u32 line = __builtin_LINE());

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

private:

    void set_cursor(i32 cursor_kind);

    bool is_valid() const { return true; }
    void invalidate() { }
    void destroy();

    u32 m_uid { 0 };

    SimpleRenderer* m_renderer { nullptr };
    FreeGlyphAtlas* m_atlas { nullptr };

    u64 m_active_id { 0 };

    Vec2f m_last_mouse_pos { 0.0f, 0.0f };
    Vec2f m_mouse_pos { 0.0f, 0.0f };

    i32 m_scroll_x { 0 };
    i32 m_scroll_y { 0 };

    i32 m_cursor_kind { -1 };

    f32 m_time { 0.0f };

    bool m_mouse_left_down : 1 { false };
};

struct Button {
    UI& ui;
    Vec4f box;

    bool hovered : 1;
    bool clicked : 1;

    void fill_rect(Vec4f color) const;
};
inline void Button::fill_rect(Vec4f color) const { ui.fill_rect(box, color); }

}

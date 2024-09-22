#pragma once
#include <Rexim/LA.h>

struct Style {
    f32 border_size() const { return 2.0f; }

    Vec4f background_color() const { return hex_to_vec4f(0x636A72FF); }

    Vec4f text_color() const { return hex_to_vec4f(0x95A99FFF); }
    Vec4f text_alternate_color() const { return hex_to_vec4f(0x707F78FF); }
    Vec4f text_info_color() const { return hex_to_vec4f(0xE4EDF1FF); }
    Vec4f text_error_color() const { return hex_to_vec4f(0xA98989FF); }

    Vec2f file_browser_font_size() const { return vec2f(0.0f, 24.0f); }
    Vec4f file_browser_color() const { return hex_to_vec4f(0x161F24FF); }
    Vec4f file_browser_border_color() const { return hex_to_vec4f(0x434C51FF); }
    f32 file_browser_indent_width() const { return 8.0f; }
    Vec4f file_browser_indent_color() const { return hex_to_vec4f(0x2A3338FF); }

    Vec2f status_bar_font_size() const { return vec2f(0.0f, 16.0f); }
    Vec4f status_bar_color() const { return hex_to_vec4f(0x3E464BFF); }
    Vec4f status_bar_border_color() const { return hex_to_vec4f(0x495257FF); }

    Vec4f toolbar_color() const { return hex_to_vec4f(0x596267FF); }
    Vec4f toolbar_border_color() const { return hex_to_vec4f(0x495257FF); }

    static Style const& the();

    Style(Style&&) = delete;
    Style& operator=(Style&&) = delete;
    Style(Style const&) = delete;
    Style& operator=(Style const&) = delete;
private:
    constexpr Style() = default;
};

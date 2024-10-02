#pragma once
#include "./Forward.h"

#include "./View.h"

#include <Rexim/LA.h>
#include <Ty/Base.h>

namespace UIView {

Text* text(StringView);
Text* text(usize);
Text* text(isize);
Text* text(f64);

[[gnu::format(printf, 1, 2)]]
Text* text(c_string, ...);

struct Text : public UIView<Text> {
    constexpr Text(StringView text)
        : m_text(text)
    {
    }

    Vec4f render(UI::UI& ui, Vec4f bounds) const override;

    StringView text() const { return m_text; }

    f64 height(UI::UI&) const override;
    f64 width(UI::UI&) const override;

    virtual f64 font_size() const { return m_font_size; }
    Self* set_font_size(f64 size)
    {
        m_font_size = size;
        return self();
    }

    virtual Vec4f text_color() const { return m_text_color; }
    Self* set_text_color(Vec4f color)
    {
        m_text_color = color;
        return self();
    }

protected:
    f64 m_font_size = 12.0f;
    Vec4f m_text_color = hex_to_vec4f(0x000000FF);

private:
    StringView m_text {};
};

}

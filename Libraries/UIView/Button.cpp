#include "./Button.h"

#include <UI/UI.h>

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

namespace UIView {

Vec4f Button::render(UI::UI& ui, Vec4f bounds) const
{
    auto button = ui.button(bounds);

    switch (button.state()) {
    case UI::ButtonState::Action:
        if (m_action_style) {
            // FIXME
            m_action_style(const_cast<Self*>(this));
        }
        if (m_action) {
            m_action();
        }
        break;
    case UI::ButtonState::Pressed:
        if (m_press_style) {
            // FIXME
            m_press_style(const_cast<Self*>(this));
        }
        break;
    case UI::ButtonState::Hovered:
        if (m_hover_style) {
            // FIXME
            m_hover_style(const_cast<Self*>(this));
        }
        if (m_hover) {
            m_hover();
        }
        break;
    case UI::ButtonState::None:
        break;
    }

    bounds = ViewBase::render(ui, bounds);
    MUST(ui.set_font_size(vec2fs(font_size())));
    ui.text(bounds, text(), text_color());

    return bounds;
}

f64 Button::width(UI::UI& ui) const
{
    if (m_width) {
        return m_width;
    }
    MUST(ui.set_font_size(vec2fs(font_size())));
    m_width = ui.measure_text(m_text).x;
    return ViewBase::width(ui);
}

f64 Button::height(UI::UI& ui) const
{
    if (m_height) {
        return m_height;
    }
    MUST(ui.set_font_size(vec2fs(font_size())));
    m_height = m_font_size;
    // FIXME: m_height = ui.measure_text(m_text).y;
    return ViewBase::height(ui);
}

Button* button(StringView text)
{
    return Button::make(text);
}

[[gnu::format(printf, 1, 2)]]
Button* button(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int size = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);
    va_start(args, fmt);

    auto content = MUST(ViewBase::alloc<char>(size + 1));
    vsnprintf(content.data(), content.size(), fmt, args);
    va_end(args);

    return button(content);
}

Button* button(usize value)
{
    return button("%" PRIuPTR, value);
}

Button* button(isize value)
{
    return button("%" PRIiPTR, value);
}

Button* button(f64 value)
{
    return button("%.2f", value);
}

}

#include "./Text.h"

#include <UI/UI.h>

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

namespace UIView {

Vec4f Text::render(UI::UI& ui, Vec4f bounds) const
{
    bounds = ViewBase::render(ui, bounds);
    ui.fill_rect(bounds, background_color());
    MUST(ui.set_font_size(vec2fs(font_size())));

    auto w = width(ui);
    auto h = height(ui);
    auto s = bounds.start_point();
    s.x += bounds.width / 2.0 - w / 2.0;
    s.y += bounds.height / 2.0 - h / 2.0;

    ui.text(vec4fv(s, vec2f(w, h)), text(), text_color());
    return bounds;
}

f64 Text::width(UI::UI& ui) const
{
    if (m_width) {
        return m_width;
    }
    MUST(ui.set_font_size(vec2fs(font_size())));
    m_width = ui.measure_text(m_text).x;
    return ViewBase::width(ui);
}

f64 Text::height(UI::UI& ui) const
{
    if (m_height) {
        return m_height;
    }
    MUST(ui.set_font_size(vec2fs(font_size())));
    m_height = m_font_size;
    // FIXME: m_height = ui.measure_text(m_text).y;
    return ViewBase::height(ui);
}

Text* text(StringView text)
{
    return Text::make(text);
}

[[gnu::format(printf, 1, 2)]]
Text* text(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int size = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);
    va_start(args, fmt);

    auto content = MUST(ViewBase::alloc<char>(size + 1));
    vsnprintf(content.data(), content.size(), fmt, args);
    va_end(args);

    return text(content);
}

Text* text(usize value)
{
    return text("%" PRIuPTR, value);
}

Text* text(isize value)
{
    return text("%" PRIiPTR, value);
}

Text* text(f64 value)
{
    return text("%.2f", value);
}

}

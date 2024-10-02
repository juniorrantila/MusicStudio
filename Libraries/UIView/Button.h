#pragma once
#include "./Forward.h"

#include "./View.h"
#include "./FrameCapture.h"

#include <Rexim/LA.h>
#include <Ty/Base.h>

namespace UIView {

Button* button(StringView);
Button* button(usize);
Button* button(isize);
Button* button(f64);

[[gnu::format(printf, 1, 2)]]
Button* button(c_string, ...);

struct Button : public UIView<Button> {
    constexpr Button(StringView text)
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

    Self* hover_style(FrameCapture<void(Self*)> builder)
    {
        m_hover_style = move(builder);
        return self();
    }

    Self* press_style(FrameCapture<void(Self*)> builder)
    {
        m_press_style = move(builder);
        return self();
    }

    Self* action_style(FrameCapture<void(Self*)> builder)
    {
        m_action_style = move(builder);
        return self();
    }

    Self* action(FrameCapture<void()>&& action)
    {
        m_action = move(action);
        return self();
    }

    Self* hover(FrameCapture<void()>&& hover)
    {
        m_hover = move(hover);
        return self();
    }

protected:
    f64 m_font_size = 12.0f;
    Vec4f m_text_color = hex_to_vec4f(0x000000FF);
    FrameCapture<void()> m_action = nullptr;
    FrameCapture<void()> m_hover = nullptr;

    FrameCapture<void(Self*)> m_hover_style = nullptr;
    FrameCapture<void(Self*)> m_press_style = nullptr;
    FrameCapture<void(Self*)> m_action_style = nullptr;

private:
    StringView m_text {};
};

}

#pragma once
#include <Ty/Base.h>
#include <Ty/View.h>
#include <Ty/Vector.h>
#include <Ty/ArenaAllocator.h>

#include <Rexim/LA.h>
#include <UI/Forward.h>

namespace UIView {

struct ViewBase {
    static ErrorOr<void*> alloc(usize size, usize align, c_string func = __builtin_FUNCTION(), c_string file = __builtin_FILE(), usize line = __builtin_LINE());
    static void drain();

    template <typename T>
    static ErrorOr<T*> alloc(c_string function = __builtin_FUNCTION(), c_string file = __builtin_FILE(), usize line = __builtin_LINE())
    {
        return (T*)TRY(alloc(sizeof(T), alignof(T), function, file, line));
    }

    template <typename T>
    static ErrorOr<View<T>> alloc(usize size, c_string func = __builtin_FUNCTION(), c_string file = __builtin_FILE(), usize line = __builtin_LINE())
    {
        auto* data = (T*)TRY(alloc(size * sizeof(T), alignof(T), func, file, line));
        return View(data, size);
    }

    virtual Vec4f render(UI::UI& ui, Vec4f bounds) const;

    virtual f64 y() const { return m_y; }
    virtual f64 x() const { return m_x; }
    virtual f64 width(UI::UI&) const { return m_width; }
    virtual f64 height(UI::UI&) const { return m_height; }
    virtual Vec4f background_color() const { return m_background_color; }
    virtual Vec4f border_left_color() const { return m_border_left_color; }
    virtual f64 border_left_width() const { return m_border_left_width; }
    virtual Vec4f border_bottom_color() const { return m_border_bottom_color; }
    virtual f64 border_bottom_width() const { return m_border_left_width; }
 
private:
    static inline ArenaAllocator s_pool {};

    static constexpr auto MiB = 1024ULL * 1024ULL;
    static ErrorOr<void> init(usize max_memory = 4 * MiB);

protected:
    mutable f64 m_x = 0.0;
    mutable f64 m_y = 0.0;
    mutable f64 m_height = 0.0;
    mutable f64 m_width = 0.0;
    mutable f64 m_border_left_width = 0.0;
    mutable f64 m_border_bottom_width = 0.0;
    mutable Vec4f m_background_color = hex_to_vec4f(0x00000000);
    mutable Vec4f m_border_left_color = hex_to_vec4f(0x00000000);
    mutable Vec4f m_border_bottom_color = hex_to_vec4f(0x00000000);
};

template <typename S>
struct UIView : public ViewBase {
    using Self = S;

    Self* self() { return (Self*)this; }

    static Self* make(Self value, c_string func = __builtin_FUNCTION(), c_string file = __builtin_FILE(), usize line = __builtin_LINE())
    {
        auto* self = MUST(ViewBase::alloc<Self>(func, file, line));
        return new (self) Self(move(value));
    }

    Self* set_x(f64 x)
    {
        m_x = x;
        return self();
    }

    Self* set_y(f64 y)
    {
        m_y = y;
        return self();
    }

    Self* set_width(f64 width)
    {
        m_width = width;
        return self();
    }

    Self* set_height(f64 height)
    {
        m_height = height;
        return self();
    }

    Self* set_background_color(Vec4f color)
    {
        m_background_color = color;
        return self();
    }

    Self* set_border_left_color(Vec4f color)
    {
        m_border_left_color = color;
        return self();
    }

    Self* set_border_left_width(f64 width)
    {
        m_border_left_width = width;
        return self();
    }

    Self* set_border_bottom_color(Vec4f color)
    {
        m_border_bottom_color = color;
        return self();
    }

    Self* set_border_bottom_width(f64 width)
    {
        m_border_bottom_width = width;
        return self();
    }
};

}

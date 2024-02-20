#pragma once
#include <Ty/ErrorOr.h>
#ifdef __APPLE__
#include <objc/runtime.h>
#endif

namespace UI {

#ifdef __APPLE__
using NativeHandle = id;
#else
using NativeHandle = void*;
#endif


struct Window {
    static ErrorOr<Window> create(StringView name, i32 x, i32 y, i32 width, i32 height);
    ~Window();

    Window(Window const&) = delete;
    Window(Window&& other)
        : m_handle(other.m_handle)
    {
        other.invalidate();
    }

    Window& operator=(Window&& other)
    {
        if (this == &other)
            return *this;
        m_handle = other.m_handle;
        other.invalidate();
        return *this;
    }

    NativeHandle native_handle() const;
    void show() const;
    void run() const;

private:
    constexpr Window(NativeHandle window)
        : m_handle(window)
    {
    }

    void invalidate() { m_handle = nullptr; }
    bool is_valid() const { return m_handle != nullptr; }

    NativeHandle m_handle { nullptr };
};

}

#pragma once
#include <Ty/ErrorOr.h>

namespace UI {

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

    void* native_handle() const;
    void show() const;
    void run() const;

private:
    constexpr Window(void* window)
        : m_handle(window)
    {
    }

    void invalidate() { m_handle = nullptr; }
    bool is_valid() const { return m_handle != nullptr; }

    void* m_handle { nullptr };
};

}

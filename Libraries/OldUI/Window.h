#pragma once
#include <Ty/ErrorOr.h>
#include <Ty/RefPtr.h>

namespace UI {

struct WindowHandle {
    virtual ~WindowHandle(){}
    virtual void* native_handle() const = 0;
    virtual void* native_window() const = 0;
    virtual void resize(i32 x, i32 y) = 0;
};

struct Window {
    static ErrorOr<RefPtr<Window>> create(StringView name, i32 x, i32 y, i32 width, i32 height);
    ~Window()
    {
        if (m_handle) {
            delete m_handle;
            invalidate();
        }
    }

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

    void* native_handle() const { return m_handle->native_handle(); }
    void* native_window() const { return m_handle->native_window(); }

    void resize(i32 x, i32 y) const { m_handle->resize(x, y); }

private:
    Window(WindowHandle* window)
        : m_handle(window)
    {
    }

    void invalidate() { m_handle = nullptr; }
    bool is_valid() const { return m_handle != nullptr; }

    WindowHandle* m_handle { nullptr };
};

}

#pragma once
#include <Ty/ErrorOr.h>
#include <Ty/SmallCapture.h>

#include "./KeyCode.h"

namespace UI {

struct Window;
struct Application {
    static ErrorOr<Application> create(StringView title, i32 x, i32 y, i32 width, i32 height);
    Application(Application const&) = delete;

    Application(Application&& other)
    {
        __builtin_memcpy(this, &other, sizeof(*this));
        other.handle_move(this);
        other.invalidate();
    }

    Application& operator=(Application&& other)
    {
        if (this == &other)
            return *this;
        this->~Application();
        __builtin_memcpy(this, &other, sizeof(*this));
        handle_move(this);
        other.invalidate();
        return *this;
    }

    ~Application();

    void run() const;

    void add_child_window(RefPtr<Window>) const;

    SmallCapture<void()> on_update;

    SmallCapture<void(f32, f32)> on_window_resize;

    SmallCapture<void(KeyCode, u32)> on_key_down;
    SmallCapture<void(KeyCode, u32)> on_key_up;

    SmallCapture<void()> on_mouse_down;
    SmallCapture<void()> on_mouse_up;
    SmallCapture<void(f32, f32)> on_mouse_move;

    SmallCapture<void(f32, f32)> on_scroll;

    f32 width() const { return m_width; }
    f32 height() const { return m_height; }

    void set_width(f32 width) { m_width = width; }
    void set_height(f32 height) { m_height = height; }

    void update() const;
private:
    Application(void* native_handle, f32 width, f32 height);

    void handle_move(Application* into);
    void invalidate() { m_native_handle = nullptr; }
    bool is_valid() const { return m_native_handle != nullptr; }

    void* m_native_handle { 0 };
    f32 m_width { 0.0f };
    f32 m_height { 0.0f };
};

}

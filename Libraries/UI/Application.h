#pragma once
#include <Rexim/LA.h>
#include <Ty/ErrorOr.h>
#include <Ty/Signal.h>
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

    SmallCapture<void(KeyCode, u32)> on_key_down;
    SmallCapture<void(KeyCode, u32)> on_key_up;

    Signal<Vec2f> window_size { 0, 0 };
    Signal<Vec2f> mouse_pos { 0, 0 };
    Signal<Vec2f> scroll { 0, 0 };
    Signal<bool> is_mouse_left_down { false };

    f32 width() const { return window_size->x; }
    f32 height() const { return window_size->y; }

    void update() const;
private:
    Application(void* native_handle, f32 width, f32 height);

    void handle_move(Application* into);
    void invalidate() { m_native_handle = nullptr; }
    bool is_valid() const { return m_native_handle != nullptr; }

    void* m_native_handle { 0 };
};

}

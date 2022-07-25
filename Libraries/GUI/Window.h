#pragma once
#include <JR/Types.h>
#include <JR/StringView.h>

#if !defined(_WIN) && !defined(__apple__)
#include <X11/Xlib.h>
#else
#error "Don't know what to include"
#endif

namespace GUI {

enum class Key {
    Number1,
    Number2,
    Number3,
    Number4,
    Number5,
    Number6,
    Number7,
    Number8,
    Number9,
    Number0,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Comma,
    Period
};

struct Window {
    static ErrorOr<Window> create(StringView name, u32 width, u32 height);
    void destroy() const;

    bool should_close();

    bool resize(i32 width, i32 height);

    void* raw_handle();

    template <typename T>
    void set_on_key_down(T* argument, void(*callback)(T*, Key))
    {
        m_key_down_argument = argument;
        m_key_down_callback = (void(*)(void*, Key))(callback);
    }

    template <typename T>
    void set_on_key_up(T* argument, void(*callback)(T*, Key))
    {
        m_key_up_argument = argument;
        m_key_up_callback = (void(*)(void*, Key))(callback);
    }

private:
    void* m_key_down_argument;
    void(*m_key_down_callback)(void*, Key);

    void* m_key_up_argument;
    void(*m_key_up_callback)(void*, Key);

#if !defined(_WIN) && !defined(__apple__)
    constexpr Window(Display* display, ::Window window)
        : m_display(display)
        , m_window(window)
    {
    }
    Display* m_display;
    ::Window m_window;
#else
#error "Don't know what to do"
#endif
};

}


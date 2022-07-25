#include <GUI/Window.h>
#include <JR/String.h>
#include <JR/Defer.h>
#include <X11/X.h>
#include <X11/Xlib.h>

namespace GUI {

#if !defined(_WIN) && !defined(__apple__)

ErrorOr<Window> Window::create(StringView name, u32 width, u32 height)
{
    auto display = XOpenDisplay(nullptr);
    // static Atom wmatom[WMLast];

    // wmatom[XEmbed] = XInternAtom(display, "_XEMBED", false);
    // XInternAtom(display, "_XEMBED", false);

    auto screen_handle = DefaultScreen(display);
    auto root_window = RootWindow(display, screen_handle);

    auto window = XCreateSimpleWindow(display, root_window, 0, 0,
        width, height, 0, 0, 0);

    // clang-format off
    XSelectInput(display, window, ExposureMask
                                | ButtonPressMask
                                | ButtonReleaseMask
                                | Button1MotionMask 
                                | Button2MotionMask
                                | Button3MotionMask
                                | Button4MotionMask 
                                | Button5MotionMask
                                | ButtonMotionMask
                                | PointerMotionMask
                                | PointerMotionHintMask
                                | KeymapStateMask
                                | OwnerGrabButtonMask
                                | EnterWindowMask
                                | LeaveWindowMask
                                | KeyPressMask
                                | KeyReleaseMask
                                | VisibilityChangeMask
                                | FocusChangeMask
                                | PropertyChangeMask
                                );
    // clang-format on
    auto name_string = TRY(name.to_string());
    Defer destroy_name_string = [&] {
        name_string.destroy();
    };
    XStoreName(display, window, name_string.as_c_string());
    XMapWindow(display, window);
    while (1) {
        XEvent e;
        XNextEvent(display, &e);
        if (e.type == Expose)
            break;
    }
    return Window { display, window };
}

void Window::destroy() const
{
    XUnmapWindow(m_display, m_window);
    XUngrabKeyboard(m_display, CurrentTime);
    XDestroyWindow(m_display, m_window);
    // XSetInputFocus(m_display, focuswin, RevertToPointerRoot, CurrentTime);
    XSync(m_display, false);
    XCloseDisplay(m_display);
}

// FIXME: Don't hardcode keycodes.
ErrorOr<Key> keycode_to_key(i32 keycode)
{
    switch (keycode) {
    case 10:
        return Key::Number1;
    case 11:
        return Key::Number2;
    case 12:
        return Key::Number3;
    case 13:
        return Key::Number4;
    case 14:
        return Key::Number5;
    case 15:
        return Key::Number6;
    case 16:
        return Key::Number7;
    case 17:
        return Key::Number8;
    case 18:
        return Key::Number9;
    case 19:
        return Key::Number0;
    case 24:
        return Key::Q;
    case 25:
        return Key::W;
    case 26:
        return Key::E;
    case 27:
        return Key::R;
    case 28:
        return Key::T;
    case 29:
        return Key::Y;
    case 30:
        return Key::U;
    case 31:
        return Key::I;
    case 32:
        return Key::O;
    case 33:
        return Key::P;
    case 38:
        return Key::A;
    case 39:
        return Key::S;
    case 40:
        return Key::D;
    case 41:
        return Key::F;
    case 42:
        return Key::G;
    case 43:
        return Key::H;
    case 44:
        return Key::J;
    case 45:
        return Key::K;
    case 46:
        return Key::L;
    case 52:
        return Key::Z;
    case 53:
        return Key::X;
    case 54:
        return Key::C;
    case 55:
        return Key::V;
    case 56:
        return Key::B;
    case 57:
        return Key::N;
    case 58:
        return Key::M;
    case 59:
        return Key::Comma;
    case 60:
        return Key::Period;
    }
    return Error::from_string_literal("invalid key");
}

// FIXME: Don't hardcode this.
static bool keycode_is_pressed[60];

bool Window::should_close()
{
    XEvent e;
    XNextEvent(m_display, &e);
    if (e.type == DestroyNotify)
        return true;
    // static auto escape_sym = XStringToKeysym("Escape");
    // static auto escape_key = XKeysymToKeycode(m_display, escape_sym);
    if (e.type == KeyRelease) {
        // __builtin_printf("key release: %d\n", e.xkey.keycode);
        auto keycode = e.xkey.keycode;
        if (keycode < 60 && keycode_is_pressed[keycode]) {
            keycode_is_pressed[keycode] = false;
            auto key = keycode_to_key(keycode);
            if (key.has_value())
                m_key_up_callback(m_key_up_argument, key.value());
        }
    }
    if (e.type == KeyPress) {
        auto keycode = e.xkey.keycode;
        if (keycode < 60 && !keycode_is_pressed[keycode]) {
            keycode_is_pressed[keycode] = true;
            auto key = keycode_to_key(keycode);
            if (key.has_value())
                m_key_down_callback(m_key_down_argument, key.value());
        }
    }

    return false;
#if 0

    static void (*handler[LASTEvent])(XEvent const*) {
        [ConfigureNotify] = configure_notify,
        [ConfigureRequest] = configure_request,
        [CreateNotify] = create_notify,
        //[MapRequest] = map_request,
        [UnmapNotify] = unmap_notify,
        [DestroyNotify] = destroy_notify,
        //[Expose] = expose,
        [FocusIn] = focus_in,
        [KeyPress] = key_press,
        [KeyRelease] = key_press
    };

    XEvent e;
    XNextEvent(dpy, &e);
    if (event_handlers[e.type])
        (event_handlers[e.type])(&e);
#endif
    return false;
}

bool Window::resize(i32 width, i32 height)
{
    return !XResizeWindow(m_display, m_window, width, height);
}

void* Window::raw_handle() { return (void*)m_window; }

#else
#error "Don't know how to represent these"
#endif

}

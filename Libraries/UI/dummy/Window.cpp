#include "../Window.h"
#include <assert.h>

namespace UI {

ErrorOr<Window> Window::create(StringView, i32, i32, i32, i32)
{
    return Error::from_string_literal("unimplemented");
}

Window::~Window()
{
}

void* Window::native_handle() const
{
    assert(false && "Window::native_handle(): unimplemented");
    return nullptr;
}

void Window::show() const
{
    assert(false && "Window::show(): unimplemented");
}

void Window::run() const
{
    assert(false && "Window::run(): unimplemented");
}

}

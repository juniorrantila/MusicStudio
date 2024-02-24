#include "../Application.h"
#include <Ty/Verify.h>

namespace UI {

ErrorOr<Application> Application::create(StringView, i32, i32, i32, i32)
{
    return Error::from_string_literal("unimplemented");
}

Application::~Application() = default;

void Application::run() const
{
    VERIFY(false && "unimplemented");
}

void Application::add_child_window(Window const&) const
{
    VERIFY(false && "unimplemented");
}

void Application::handle_move(Application*)
{
    VERIFY(false && "unimplemented");
}

}

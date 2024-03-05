#include "../Window.h"
#include <Ty/RefPtr.h>
#include <assert.h>

namespace UI {

ErrorOr<RefPtr<Window>> Window::create(StringView, i32, i32, i32, i32)
{
    return Error::from_string_literal("unimplemented");
}

}

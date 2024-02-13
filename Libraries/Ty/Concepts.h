#pragma once
#include "Base.h"
#include "Forward.h"
#include "Traits.h"

namespace Ty {

template <typename T>
concept Writable = requires(T& instance, StringView view)
{
    is_same<decltype(instance.write(view, view, view, view)),
        ErrorOr<u32>>;
    is_same<decltype(instance.writeln(view, view, view, view)),
        ErrorOr<u32>>;
};

}

using namespace Ty;

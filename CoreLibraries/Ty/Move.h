#pragma once
#include "Traits.h"

namespace Ty {

template<typename T>
constexpr T&& forward(RemoveReference<T>& param)
{
    return static_cast<T&&>(param);
}

template<typename T>
constexpr T&& forward(RemoveReference<T>&& param) noexcept
{
    static_assert(!IsLvalueReference<T>, "Can't forward an rvalue as an lvalue.");
    return static_cast<T&&>(param);
}

template<typename T>
constexpr T&& move(T& arg)
{
    return static_cast<T&&>(arg);
}

};

using Ty::move;
using Ty::forward;

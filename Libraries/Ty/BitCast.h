#pragma once

namespace Ty {

template <typename T, typename U>
[[nodiscard]] constexpr inline T bit_cast(U const& a)
{
    return __builtin_bit_cast(T, a);
}

}

using Ty::bit_cast;

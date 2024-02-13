#pragma once
#include "Base.h"
#include "Verify.h"

namespace Ty {

struct Bytes {
    constexpr Bytes(void const* data, usize size)
        : data((u8*)data)
        , size(size)
    {
    }

    constexpr u8 const* begin() const { return data; }
    constexpr u8 const* end() const { return &data[size]; }

    constexpr u8 operator[](usize index) const
    {
        VERIFY(index < size);
        return data[index];
    }

    u8 const* data;
    usize size;
};

}

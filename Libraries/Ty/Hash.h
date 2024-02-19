#pragma once
#include "./Base.h"

namespace Ty {

struct Hash {
    constexpr u64 hash() const { return m_hash; }

    constexpr Hash& djbd(i8 number)
    {
        return djbd(&number, sizeof(number));
    }

    constexpr Hash& djbd(u8 number)
    {
        return djbd(&number, sizeof(number));
    }

    constexpr Hash& djbd(i16 number)
    {
        return djbd(&number, sizeof(number));
    }

    constexpr Hash& djbd(u16 number)
    {
        return djbd(&number, sizeof(number));
    }

    constexpr Hash& djbd(i32 number)
    {
        return djbd(&number, sizeof(number));
    }

    constexpr Hash& djbd(u32 number)
    {
        return djbd(&number, sizeof(number));
    }

    constexpr Hash& djbd(i64 number)
    {
        return djbd(&number, sizeof(number));
    }

    constexpr Hash& djbd(u64 number)
    {
        return djbd(&number, sizeof(number));
    }

    constexpr Hash& djbd(void const* buffer, usize size)
    {
        u8 const* bytes = (u8 const*)buffer;
        for (usize i = 0; i < size; ++i) {
            m_hash = m_hash * 33 + bytes[i];
        }
        return *this;
    }

private:
    u64 m_hash { 5381 };
};

}

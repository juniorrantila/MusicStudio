#pragma once
#include "./Base.h"

static constexpr u64 KiB = 1024;
static constexpr u64 MiB = 1024 * KiB;
static constexpr u64 GiB = 1024 * KiB;
static constexpr u64 TiB = 1024 * GiB;

#ifdef __cplusplus

template <typename T, typename U>
static constexpr T ty_bit_cast(U other)
{
    return __builtin_bit_cast(T, other);
}

static constexpr u8 ty_bits(u8 v) { return v; }

static constexpr u8 ty_bits_fitting(u64 v)
{
    u8 bits = 0;
    while (v != 0) {
        bits += 1;
        v /= 2;
    }
    return bits;
}

static consteval u64 ty_bituint_max(u8 bits)
{
    if (bits == 64) return (u64)-1;
    return (1LLU << (bits + 1)) - 1;
}

#endif

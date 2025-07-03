#pragma once
#include "./Base.h"

static constexpr u64 KiB = 1024;
static constexpr u64 MiB = 1024 * KiB;
static constexpr u64 GiB = 1024 * KiB;
static constexpr u64 TiB = 1024 * GiB;

C_INLINE u16 ty_device_endian_from_u16le(u16 value)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return value;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return __builtin_bswap16(value);
#else
#error "unknown endianness"
#endif
}

C_INLINE u32 ty_device_endian_from_u32le(u32 value)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return value;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return __builtin_bswap32(value);
#else
#error "unknown endianness"
#endif
}

C_INLINE u64 ty_device_endian_from_u64le(u64 value)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return value;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return __builtin_bswap64(value);
#else
#error "unknown endianness"
#endif
}

C_INLINE u16 ty_device_endian_from_u16be(u16 value)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap16(value);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return value;
#else
#error "unknown endianness"
#endif
}

C_INLINE u32 ty_device_endian_from_u32be(u32 value)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap32(value);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return value;
#else
#error "unknown endianness"
#endif
}

C_INLINE u64 ty_device_endian_from_u64be(u64 value)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap64(value);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return value;
#else
#error "unknown endianness"
#endif
}

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

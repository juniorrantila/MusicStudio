#pragma once
#include "./Base.h"

C_INLINE bool bit_is_set(u8 const* bitset, u64 index)
{
    return (bitset[index / 8] & (1 << index % 8)) != 0;
}

C_INLINE bool bit_set(u8* bitset, u64 index, bool value)
{
    bool old_value = bit_is_set(bitset, index);
    if (value) {
        bitset[index / 8] |= (1 << (index % 8));
        return old_value;
    }
    bitset[index / 8] &= ~(1 << (index % 8));
    return old_value;
}

C_INLINE u64 bit_popcount(u8 const* bitset, u64 byte_count)
{
    u64 bit_count = byte_count * 8;
    u64 count = 0;
    for (u64 i = 0; i < bit_count; i += 8) {
        count += bit_is_set(bitset, i + 0);
        count += bit_is_set(bitset, i + 1);
        count += bit_is_set(bitset, i + 2);
        count += bit_is_set(bitset, i + 3);
        count += bit_is_set(bitset, i + 4);
        count += bit_is_set(bitset, i + 5);
        count += bit_is_set(bitset, i + 6);
        count += bit_is_set(bitset, i + 7);
    }
    return count;
}

C_INLINE u64 popcount_u64(u64 bitset)
{
    return bit_popcount((u8 const*)&bitset, sizeof(bitset));
}

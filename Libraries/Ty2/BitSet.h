#pragma once
#include "./Base.h"

C_API inline bool bit_is_set(u8 const* bitset, usize index)
{
    return (bitset[index / 8] & (1 << index % 8)) != 0;
}

C_API inline bool bit_set(u8* bitset, usize index, bool value)
{
    bool old_value = bit_is_set(bitset, index);
    if (value) {
        bitset[index / 8] |= (1 << index);
        return old_value;
    }
    bitset[index / 8] &= ~(1 << index);
    return old_value;
}

C_API inline usize bit_popcount(u8 const* bitset, usize byte_count)
{
    usize bit_count = byte_count * 8;
    usize count = 0;
    for (usize i = 0; i < bit_count; i += 8) {
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

C_API inline usize usize_popcount(usize bitset)
{
    return bit_popcount((u8 const*)&bitset, sizeof(bitset));
}

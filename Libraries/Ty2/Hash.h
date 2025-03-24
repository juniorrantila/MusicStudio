#pragma once
#include "./Base.h"

static const usize djb2_initial_seed = 5381;
C_API inline usize djb2(usize seed, void const* value, usize size)
{
    u8 const* data = (u8 const*)value;
    usize hash = seed;
    int c = 0;

    for (usize i = 0; i < size; i++) {
        c = data[i];
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash;
}

static const usize sdbm_initial_seed = 5381;
C_API inline usize sdbm(usize seed, void const* value, usize size)
{
    u8 const* data = (u8 const*)value;
    usize hash = seed;
    int c = 0;

    for (usize i = 0; i < size; i++) {
        c = data[i];
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

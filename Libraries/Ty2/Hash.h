#pragma once
#include "./Base.h"

static const u64 djb2_initial_seed = 5381;
C_INLINE u64 djb2(u64 seed, void const* value, u64 size)
{
    u8 const* data = (u8 const*)value;
    u64 hash = seed;
    int c = 0;

    for (u64 i = 0; i < size; i++) {
        c = data[i];
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash;
}
C_INLINE u64 djb2_u64(u64 seed, u64 value)
{
    return djb2(seed, &value, sizeof(value));
}

static const u64 sdbm_initial_seed = 5381;
C_INLINE u64 sdbm(u64 seed, void const* value, u64 size)
{
    u8 const* data = (u8 const*)value;
    u64 hash = seed;
    int c = 0;

    for (u64 i = 0; i < size; i++) {
        c = data[i];
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

C_INLINE u64 sdbm_u64(u64 seed, u64 value)
{
    return sdbm(seed, &value, sizeof(value));
}

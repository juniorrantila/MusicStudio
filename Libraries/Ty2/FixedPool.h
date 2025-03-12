#pragma once
#include "./Allocator.h"

typedef struct FixedPool {
    Allocator allocator;

    usize object_size;
    usize object_align;
    u8* objects;
    u8* object_allocated;

#ifdef __cplusplus
    static FixedPool from_slice(usize object_size, usize object_align, u8* memory, usize memory_size);

    usize bytes_used() const;
    void drain();

    void* alloc(usize byte_size);
    void free(void* ptr, usize byte_size);
    bool owns(void* ptr) const;
#endif
} FixedPool;

C_API FixedPool fixed_pool_from_slice(usize object_size, usize object_align, u8* memory, usize memory_size);

C_API usize fixed_pool_bytes_used(FixedPool const*);
C_API void fixed_pool_drain(FixedPool*);

C_API void* fixed_pool_alloc(FixedPool*, usize byte_size);
C_API void fixed_pool_free(FixedPool*, void* ptr, usize byte_size);
C_API bool fixed_pool_owns(FixedPool const*, void* ptr);

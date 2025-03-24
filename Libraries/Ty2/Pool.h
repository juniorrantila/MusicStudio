#pragma once
#include "./Base.h"

#include "./Allocator.h"

#include "./FixedPool.h"

typedef struct PoolSegment {
    struct PoolSegment* previous;
    struct PoolSegment* next;
    FixedPool pool;
    usize segment_size;

    alignas(16) u8 buffer[];
} PoolSegment;

typedef struct Pool {
    Allocator allocator;
    Allocator* backing_gpa;

    usize object_size;
    usize object_align;
    PoolSegment* current;

#ifdef __cplusplus
    static Pool create(Allocator* backing_gpa, usize object_size, usize object_align);
    void destroy();

    usize bytes_used() const;
    void drain();

    RETURNS_SIZED_BY(2)
    void* alloc(usize bytes);
    void free(void* ptr, usize byte_size);
    bool owns(void* ptr) const;
#endif
} Pool;

C_API Pool pool_create(Allocator* backing_gpa, usize object_size, usize object_align);
C_API void pool_destroy(Pool*);

C_API usize pool_bytes_used(Pool const*);
C_API void pool_drain(Pool*);

RETURNS_SIZED_BY(2)
C_API void* pool_alloc(Pool*, usize byte_size);
C_API void pool_free(Pool*, void* ptr, usize byte_size);
C_API bool pool_owns(Pool const*, void* ptr);

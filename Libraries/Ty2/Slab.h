#pragma once
#include "./Allocator.h"

#include "./Pool.h"

#define SLAB_POOL_COUNT 16

typedef struct Slab {
    Allocator allocator;
    Allocator* backing_gpa;

    Pool small_pools[SLAB_POOL_COUNT];
    Pool large_pools[SLAB_POOL_COUNT];

#ifdef __cplusplus
    static Slab create(Allocator* gpa);
    void destroy();

    usize bytes_used() const;
    void drain();

    bool owns(void* ptr) const;
    void* alloc(usize size, usize align);
    void free(void* ptr, usize size, usize align);
#endif
} Slab;

C_API Slab slab_create(Allocator* gpa);
C_API void slab_destroy(Slab*);

C_API usize slab_bytes_used(Slab const*);
C_API void slab_drain(Slab*);

C_API bool slab_owns(Slab const*, void* ptr);
C_API void* slab_alloc(Slab*, usize size, usize align);
C_API void slab_free(Slab*, void* ptr, usize size, usize align);

C_API usize slab_max_alignment(void);

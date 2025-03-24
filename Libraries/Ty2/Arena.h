#pragma once
#include "./Allocator.h"

#include "./FixedArena.h"
#include "./Base.h"

typedef struct ArenaSegment {
    struct ArenaSegment* previous;
    struct ArenaSegment* next;
    FixedArena arena;
    usize segment_size;

    alignas(16) u8 buffer[];
} ArenaSegment;

typedef struct { u8* value; } ArenaMark;
C_API inline ArenaMark make_arena_mark(u8* value) { return (ArenaMark){value}; }

typedef struct Arena {
    Allocator allocator;
    Allocator* backing_gpa;

    ArenaSegment* current;

#ifdef __cplusplus
    static Arena create(Allocator* gpa);
    void destroy();

    usize bytes_used() const;
    void drain();

    ArenaMark mark() const;
    void sweep(ArenaMark mark);

    bool owns(void* ptr) const;

    RETURNS_SIZED_AND_ALIGNED_BY(2, 3)
    void* alloc(usize size, usize align);
    void free(void* ptr, usize size, usize align);
#endif
} Arena;

C_API Arena arena_create(Allocator* gpa);
C_API void arena_destroy(Arena*);

C_API usize arena_bytes_used(Arena const*);
C_API void arena_drain(Arena*);

C_API ArenaMark arena_mark(Arena const*);
C_API void arena_sweep(Arena*, ArenaMark);

C_API bool arena_owns(Arena const*, void* ptr);

RETURNS_SIZED_AND_ALIGNED_BY(2, 3)
C_API void* arena_alloc(Arena*, usize size, usize align);

C_API void arena_free(Arena*, void* ptr, usize size, usize align);

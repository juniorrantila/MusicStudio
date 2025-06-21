#pragma once
#include "./Allocator.h"

typedef struct { u8* value; } FixedMark;
C_API inline FixedMark make_fixed_mark(u8* value) { return (FixedMark){value}; }

typedef struct FixedArena {
    Allocator allocator;
    u8* base;
    u8* head;
    u8* end;

#ifdef __cplusplus
    static FixedArena from_slice(void* memory, usize size);
    usize bytes_used() const;

    void drain();

    FixedMark mark() const;
    void sweep(FixedMark mark);

    bool owns(void* ptr) const;

    void* alloc(usize size, usize align);
    void free(void* ptr, usize size, usize align);
#endif
} FixedArena;

C_API FixedArena fixed_arena_from_slice(void* memory, usize size);

C_API usize fixed_arena_bytes_used(FixedArena const*);
C_API void fixed_arena_drain(FixedArena*);

C_API FixedMark fixed_arena_mark(FixedArena const*);
C_API void fixed_arena_sweep(FixedArena*, FixedMark mark);

C_API bool fixed_arena_owns(FixedArena const*, void* ptr);

C_API void* fixed_arena_alloc(FixedArena*, usize size, usize align);
C_API void fixed_arena_free(FixedArena*, void* ptr, usize size, usize align);

#pragma once
#include "./Allocator.h"

#include <stdarg.h>

typedef struct { u8* value; } FixedMark;
C_API inline FixedMark make_fixed_mark(u8* value) { return (FixedMark){value}; }

typedef struct FixedArena {
    Allocator allocator;
    u8* base;
    u8* head;
    u8* end;
    u64 allocation_count;
    u64 largest_size;

#ifdef __cplusplus
    u64 bytes_used() const;
    u64 bytes_left() const;

    void drain();

    FixedMark mark() const;
    void sweep(FixedMark mark);

    void* push(u64 size, u64 align);

    [[gnu::format(printf, 2, 3)]]
    c_string fmt(c_string, ...);

    [[gnu::format(printf, 2, 0)]]
    c_string vfmt(c_string, va_list);

    [[gnu::format(printf, 2, 3)]]
    c_string must_fmt(c_string, ...);

    [[gnu::format(printf, 2, 0)]]
    c_string must_vfmt(c_string, va_list);
#endif
} FixedArena;

C_API FixedArena fixed_arena_init(void* memory, u64 size);
C_API [[nodiscard]] bool fixed_arena_init_with_capacity(Allocator*, u64 size, u64 align, FixedArena*);

C_API u64 fixed_arena_bytes_used(FixedArena const*);
C_API u64 fixed_arena_bytes_left(FixedArena const*);
C_API void fixed_arena_drain(FixedArena*);

C_API FixedMark fixed_arena_mark(FixedArena const*);
C_API void fixed_arena_sweep(FixedArena*, FixedMark mark);

C_API void* fixed_arena_push(FixedArena*, u64 size, u64 align);

C_API [[gnu::format(printf, 2, 3)]]
c_string fixed_arena_fmt(FixedArena*, c_string, ...);

C_API [[gnu::format(printf, 2, 0)]]
c_string fixed_arena_vfmt(FixedArena*, c_string, va_list);

C_API [[gnu::format(printf, 2, 3)]]
c_string fixed_arena_must_fmt(FixedArena*, c_string, ...);

C_API [[gnu::format(printf, 2, 0)]]
c_string fixed_arena_must_vfmt(FixedArena*, c_string, va_list);

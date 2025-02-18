#include "./FixedArena.h"

#include "./Allocator.h"
#include "./Verify.h"

static void* ialloc(Allocator*, usize size, usize align);
static void ifree(Allocator*, void* ptr, usize size);

FixedArena fixed_arena_from_slice(void* memory, usize size)
{
    return (FixedArena){
        .allocator = {
            .ialloc = ialloc,
            .ifree = ifree,
        },
        .base = memory,
        .head = memory,
        .end = ((u8*)memory) + size,
    };
}

usize fixed_arena_bytes_used(FixedArena const* arena)
{
    return arena->head - arena->base;
}

void fixed_arena_drain(FixedArena* arena)
{
    arena->head = arena->base;
}

void* fixed_arena_mark(FixedArena const* arena)
{
    return arena->head;
}

void fixed_arena_sweep(FixedArena* arena, void* mark)
{
    VERIFY(fixed_arena_owns(arena, mark));
    arena->head = mark;
}

static void* ialloc(Allocator* allocator, usize size, usize align)
{
    FixedArena* arena = ty_field_base(FixedArena, allocator, allocator);
    uptr align_diff = ((uptr)arena->head) % align;
    if ((arena->head + align_diff + size) > arena->end) {
        return 0;
    }
    arena->head += align_diff;
    void* ptr = arena->head;
    arena->head += size;
    __builtin_memset(ptr, 0xAB, size);
    return ptr;
}

static void ifree(Allocator* allocator, void* ptr, usize size)
{
    FixedArena* arena = ty_field_base(FixedArena, allocator, allocator);
    VERIFY(fixed_arena_owns(arena, ptr));
    VERIFY(arena->base <= (arena->head - size));
    __builtin_memset(ptr, 0xAB, size);
    if (arena->head - size == ptr) {
        arena->head -= size;
    }
}

bool fixed_arena_owns(FixedArena const* arena, void* ptr)
{
    u8* m = (u8*)ptr;
    return m <= arena->end && m >= arena->base;
}

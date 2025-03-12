#include "./FixedArena.h"

#include "./Allocator.h"
#include "./Base.h"
#include "./Verify.h"

static void* dispatch(Allocator*, AllocatorEvent);

FixedArena FixedArena::from_slice(void* memory, usize size) { return fixed_arena_from_slice(memory, size); }
C_API FixedArena fixed_arena_from_slice(void* memory, usize size)
{
    return {
        .allocator = make_allocator(dispatch),
        .base = (u8*)memory,
        .head = (u8*)memory,
        .end = ((u8*)memory) + size,
    };
}

usize FixedArena::bytes_used() const { return fixed_arena_bytes_used(this); }
C_API usize fixed_arena_bytes_used(FixedArena const* arena)
{
    return arena->head - arena->base;
}

void FixedArena::drain() { return fixed_arena_drain(this); }
C_API void fixed_arena_drain(FixedArena* arena)
{
    arena->head = arena->base;
}

FixedMark FixedArena::mark() const { return fixed_arena_mark(this); }
C_API FixedMark fixed_arena_mark(FixedArena const* arena)
{
    return make_fixed_mark(arena->head);
}

void FixedArena::sweep(FixedMark mark) { return fixed_arena_sweep(this, mark); }
C_API void fixed_arena_sweep(FixedArena* arena, FixedMark mark)
{
    VERIFY(arena->owns(mark.value));
    arena->head = mark.value;
}

void* FixedArena::alloc(usize size, usize align) { return fixed_arena_alloc(this, size, align); }
C_API void* fixed_arena_alloc(FixedArena* arena, usize size, usize align)
{
    uptr align_diff = ((uptr)arena->head) % align;
    if ((arena->head + align_diff + size) > arena->end) {
        return 0;
    }
    arena->head += align_diff;
    void* ptr = arena->head;
    arena->head += size;
    return ptr;
}

void FixedArena::free(void* ptr, usize size, usize align) { return fixed_arena_free(this, ptr, size, align); }
C_API void fixed_arena_free(FixedArena* arena, void* ptr, usize size, usize align)
{
    (void)align;
    VERIFY(arena->owns(ptr));
    VERIFY(arena->base <= (arena->head - size));
    if (arena->head - size == ptr) {
        arena->head -= size;
    }
}

bool FixedArena::owns(void* ptr) const { return fixed_arena_owns(this, ptr); }
bool fixed_arena_owns(FixedArena const* arena, void* ptr)
{
    u8* m = (u8*)ptr;
    return m <= arena->end && m >= arena->base;
}

static void* dispatch(Allocator* a, AllocatorEvent event)
{
    FixedArena* arena = field_base(FixedArena, allocator, a);
    switch (event.tag) {
    case AllocatorEventTag_Alloc:
        return arena->alloc(event.byte_count, event.align);
    case AllocatorEventTag_Free:
        arena->free(event.ptr, event.byte_count, event.align);
        return nullptr;
    case AllocatorEventTag_Owns:
        return arena->owns(event.ptr) ? (void*)1 : 0;
    }
    return nullptr;
}

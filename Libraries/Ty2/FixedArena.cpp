#include "./FixedArena.h"

#include "./Allocator.h"
#include "./Base.h"
#include "./Verify.h"

static void* dispatch(Allocator*, AllocatorEvent);

FixedArena FixedArena::from_slice(void* memory, usize size) { return fixed_arena_from_slice(memory, size); }
C_API FixedArena fixed_arena_from_slice(void* memory, usize size)
{
    VERIFY(memory != nullptr);
    VERIFY(size != 0);
    memset_canary(memory, size);
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
    VERIFY(arena->head >= arena->base);
    uptr size = arena->head - arena->base;
    arena->head = arena->base;
    if (arena->head) memset_canary(arena->head, size);
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
    VERIFY(arena->head >= mark.value);
    uptr size = mark.value - arena->head;
    if (mark.value) memset_canary(mark.value, size);
    arena->head = mark.value;
}

void* FixedArena::alloc(usize size, usize align) { return fixed_arena_alloc(this, size, align); }
C_API void* fixed_arena_alloc(FixedArena* arena, usize size, usize align)
{
    u8* new_head = __builtin_align_up(arena->head, align);
    usize size_until_aligned = new_head - arena->head;
    if ((new_head + size) > arena->end) {
        return nullptr;
    }
    memcheck_canary(arena->head, size_until_aligned);
    memcheck_canary(new_head, size);
    arena->head = new_head;
    VERIFY(((uptr)arena->head) % align == 0);
    void* ptr = arena->head;
    arena->head += size;
    return ptr;
}

void FixedArena::free(void* ptr, usize size, usize align) { return fixed_arena_free(this, ptr, size, align); }
C_API void fixed_arena_free(FixedArena* arena, void* ptr, usize size, usize align)
{
    (void)align;
    VERIFYS(ptr != nullptr, "trying to free null pointer");
    VERIFY(arena->owns(ptr));
    VERIFY(arena->base <= (arena->head - size));
    if (arena->head - size == ptr) {
        arena->head -= size;
        memset_canary(ptr, size);
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

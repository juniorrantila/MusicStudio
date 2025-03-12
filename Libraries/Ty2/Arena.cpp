#include "./Arena.h"

#include "./Allocator.h"
#include "./Verify.h"

static void* dispatch(Allocator*, AllocatorEvent);
static ArenaSegment* create_segment(Allocator*, ArenaSegment* previous, usize segment_size);

static const usize min_segment_size = 1ULL * 1024ULL * 1024ULL;

Arena Arena::create(Allocator *gpa) { return arena_create(gpa); }
C_API Arena arena_create(Allocator* gpa)
{
    return (Arena){
        .allocator = make_allocator(dispatch),
        .backing_gpa = gpa,
        .current = 0,
    };
}

void Arena::destroy() { arena_destroy(this); }
C_API void arena_destroy(Arena* arena)
{
    ArenaSegment* seg = arena->current;
    if (!seg) return;
    while (seg->next) seg = seg->next;
    while (seg) {
        ArenaSegment* previous = seg->previous;
        memfree(arena->backing_gpa, seg, sizeof(*seg) + seg->segment_size, alignof(ArenaSegment));
        seg = previous;
    }
    *arena = (Arena){};
}

usize Arena::bytes_used() const { return arena_bytes_used(this); }
usize arena_bytes_used(Arena const* arena)
{
    usize size = 0;
    for (ArenaSegment* seg = arena->current; seg; seg = seg->previous) {
        size += seg->arena.bytes_used();
    }
    return size;
}

void Arena::drain() { arena_drain(this); }
C_API void arena_drain(Arena* arena)
{
    if (!arena->current) return;
    ArenaSegment* seg = arena->current;
    for (; seg->previous; seg = seg->previous) {
        seg->arena.drain();
    }
    seg->arena.drain();
    arena->current = seg;
}

ArenaMark Arena::mark() const { return arena_mark(this); }
C_API ArenaMark arena_mark(Arena const* arena)
{
    if (!arena->current) {
        return make_arena_mark(0);
    }
    return make_arena_mark(arena->current->arena.mark().value);
}

void Arena::sweep(ArenaMark mark) { return arena_sweep(this, mark); }
C_API void arena_sweep(Arena* arena, ArenaMark mark)
{
    if (!mark.value) {
        arena->drain();
        return;
    }
    VERIFY(arena->owns(mark.value));
    for (ArenaSegment* seg = arena->current; seg; seg = seg->previous) {
        if (!seg->arena.owns(mark.value)) {
            seg->arena.drain();
            continue;
        }
        seg->arena.sweep(make_fixed_mark(mark.value));
        arena->current = seg;
        break;
    }
}

bool Arena::owns(void* ptr) const { return arena_owns(this, ptr); }
bool arena_owns(Arena const* arena, void* ptr)
{
    if (!ptr) {
        return true;
    }
    for (ArenaSegment* seg = arena->current; seg; seg = seg->previous) {
        if (seg->arena.owns(ptr)) {
            return true;
        }
    }
    return false;
}

void* Arena::alloc(usize size, usize align) { return arena_alloc(this, size, align); }
void* arena_alloc(Arena* arena, usize size, usize align)
{
    usize segment_size = size * 2 > min_segment_size ? size * 2 : min_segment_size;

    if (!arena->current) {
        arena->current = create_segment(arena->backing_gpa, 0, segment_size);
        if (!arena->current) {
            return 0;
        }
    }

    for (ArenaSegment* seg = arena->current; seg; seg = seg->next) {
        arena->current = seg;
        void* ptr = seg->arena.alloc(size, align);
        if (ptr) return ptr;
    }

    ArenaSegment* seg = create_segment(arena->backing_gpa, arena->current, segment_size);
    if (!seg) {
        return 0;
    }
    arena->current->next = seg;
    arena->current = seg;
    return seg->arena.alloc(size, align);
}

void Arena::free(void* ptr, usize size, usize align) { return arena_free(this, ptr, size, align); }
void arena_free(Arena* arena, void* ptr, usize size, usize align)
{
    if (!arena->current)
        return;
    VERIFY(arena->owns(ptr));
    VERIFY(arena->owns(((u8*)ptr) + size));
    if (arena->current->arena.owns(ptr)) {
        arena->current->arena.free(ptr, size, align);
    }
}

static ArenaSegment* create_segment(Allocator* a, ArenaSegment* previous, usize segment_size)
{
    ArenaSegment* segment = (ArenaSegment*)memalloc(a, sizeof(ArenaSegment) + segment_size, alignof(ArenaSegment));
    if (!segment) return 0;

    *segment = (ArenaSegment){
        .previous = previous,
        .next = 0,
        .arena = FixedArena::from_slice(segment->buffer, segment_size),
        .segment_size = segment_size,
    };
    return segment;
}

static void* dispatch(Allocator* a, AllocatorEvent event)
{
    Arena* arena = field_base(Arena, allocator, a);
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

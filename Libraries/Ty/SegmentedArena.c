#include "./SegmentedArena.h"

#include "./Verify.h"
#include "./Allocator.h"

static void* ialloc(Allocator*, usize size, usize align);
static void ifree(Allocator*, void* ptr, usize size);
static ArenaSegment* create_segment(Allocator*, ArenaSegment* previous, usize segment_size);

usize min_segment_size = 1ULL * 1024ULL * 1024ULL;

SegmentedArena segmented_arena_create(Allocator* backing_allocator)
{
    return (SegmentedArena){
        .allocator = {
            .ialloc = ialloc,
            .ifree = ifree,
        },
        .backing_allocator = backing_allocator,
        .current = 0,
    };
}

void segmented_arena_destroy(SegmentedArena* arena)
{
    ArenaSegment* seg = arena->current;
    if (!seg) return;
    while (seg->next) seg = seg->next;
    while (seg) {
        ArenaSegment* previous = seg->previous;
        ty_free(arena->backing_allocator, seg, sizeof(*seg) + seg->segment_size);
        seg = previous;
    }
    *arena = (SegmentedArena){ 0 };
}

void segmented_arena_drain(SegmentedArena* arena)
{
    if (!arena->current) return;
    ArenaSegment* seg = arena->current;
    for (; seg->previous; seg = seg->previous)
        fixed_arena_drain(&seg->arena);
    fixed_arena_drain(&seg->arena);
    arena->current = seg;
}

void* segmented_arena_mark(SegmentedArena const* arena)
{
    if (!arena->current) {
        return 0;
    }
    return fixed_arena_mark(&arena->current->arena);
}

void segmented_arena_sweep(SegmentedArena* arena, void* mark)
{
    if (!mark) {
        segmented_arena_drain(arena);
        return;
    }
    VERIFY(segmented_arena_owns(arena, mark));
    for (ArenaSegment* seg = arena->current; seg; seg = seg->previous) {
        if (!fixed_arena_owns(&seg->arena, mark)) {
            fixed_arena_drain(&seg->arena);;
            continue;
        }
        fixed_arena_sweep(&seg->arena, mark);
        arena->current = seg;
        break;
    }
}

bool segmented_arena_owns(SegmentedArena const* arena, void* ptr)
{
    if (!ptr) {
        return true;
    }
    for (ArenaSegment* seg = arena->current; seg; seg = seg->previous) {
        if (fixed_arena_owns(&seg->arena, ptr)) {
            return true;
        }
    }
    return false;
}

static void* ialloc(Allocator* allocator, usize size, usize align)
{
    usize segment_size = size * 2 > min_segment_size ? size * 2 : min_segment_size;

    SegmentedArena* arena = ty_field_base(SegmentedArena, allocator, allocator);
    if (!arena->current) {
        arena->current = create_segment(arena->backing_allocator, 0, segment_size);
        if (!arena->current) {
            return 0;
        }
    }

    for (ArenaSegment* seg = arena->current; seg; seg = seg->next) {
        arena->current = seg;
        void* ptr = ty_alloc(&seg->arena.allocator, size, align);
        if (ptr) return ptr;
    }

    ArenaSegment* seg = create_segment(arena->backing_allocator, arena->current, segment_size);
    if (!seg) {
        return 0;
    }
    arena->current->next = seg;
    arena->current = seg;
    void* ptr = ty_alloc(&seg->arena.allocator, size, align);
    return ptr;
}

static void ifree(Allocator* allocator, void* ptr, usize size)
{
    SegmentedArena* arena = ty_field_base(SegmentedArena, allocator, allocator);
    if (!arena->current)
        return;
    VERIFY(segmented_arena_owns(arena, ptr));
    VERIFY(segmented_arena_owns(arena, ((u8*)ptr) + size));
    if (fixed_arena_owns(&arena->current->arena, ptr)) {
        ty_free(&arena->current->arena.allocator, ptr, size);
    }
}

static ArenaSegment* create_segment(Allocator* allocator, ArenaSegment* previous, usize segment_size)
{
    ArenaSegment* segment = (ArenaSegment*)ty_alloc(allocator, sizeof(ArenaSegment) + segment_size, alignof(ArenaSegment));
    if (!segment) return 0;

    *segment = (ArenaSegment){
        .previous = previous,
        .next = 0,
        .segment_size = segment_size,
        .arena = fixed_arena_from_slice(segment->buffer, segment_size),
    };
    return segment;
}

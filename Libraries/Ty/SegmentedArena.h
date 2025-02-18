#pragma once
#include "./Allocator.h"

#include "./FixedArena.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ArenaSegment {
    struct ArenaSegment* previous;
    struct ArenaSegment* next;
    FixedArena arena;
    usize segment_size;

    _Alignas(16) u8 buffer[];
} ArenaSegment;

typedef struct {
    Allocator allocator;
    Allocator* backing_allocator;

    ArenaSegment* current;
} SegmentedArena;

SegmentedArena segmented_arena_create(Allocator* backing_allocator);
void segmented_arena_destroy(SegmentedArena*);

void segmented_arena_drain(SegmentedArena*);
void* segmented_arena_mark(SegmentedArena const*);
void segmented_arena_sweep(SegmentedArena*, void* mark);
bool segmented_arena_owns(SegmentedArena const*, void* ptr);

#ifdef __cplusplus
}
#endif

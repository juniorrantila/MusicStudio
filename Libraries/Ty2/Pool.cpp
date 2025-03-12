#include "./Pool.h"

#include "./Allocator.h"
#include "./Verify.h"

static void* dispatch(Allocator*, AllocatorEvent);
static PoolSegment* create_segment(Allocator*, PoolSegment* previous, usize segment_size, usize object_size, usize object_align);

static const usize min_segment_size = 1ULL * 1024ULL * 1024ULL;

Pool Pool::create(Allocator *gpa, usize object_size, usize object_align) { return pool_create(gpa, object_size, object_align); }
C_API Pool pool_create(Allocator* gpa, usize object_size, usize object_align)
{
    return (Pool){
        .allocator = make_allocator(dispatch),
        .backing_gpa = gpa,
        .object_size = object_size,
        .object_align = object_align,
        .current = 0,
    };
}

void Pool::destroy() { pool_destroy(this); }
C_API void pool_destroy(Pool* pool)
{
    PoolSegment* seg = pool->current;
    if (!seg) return;
    while (seg->next) seg = seg->next;
    while (seg) {
        PoolSegment* previous = seg->previous;
        memfree(pool->backing_gpa, seg, sizeof(*seg) + seg->segment_size, alignof(PoolSegment));
        seg = previous;
    }
    *pool = (Pool){};
}

usize Pool::bytes_used() const { return pool_bytes_used(this); }
C_API usize pool_bytes_used(Pool const* pool)
{
    usize size = 0;
    for (PoolSegment* seg = pool->current; seg; seg = seg->previous) {
        size += seg->pool.bytes_used();
    }
    return size;
}

void Pool::drain() { pool_drain(this); }
C_API void pool_drain(Pool* pool)
{
    if (!pool->current) return;
    PoolSegment* seg = pool->current;
    for (; seg->previous; seg = seg->previous) {
        seg->pool.drain();
    }
    seg->pool.drain();
    pool->current = seg;
}

bool Pool::owns(void* ptr) const { return pool_owns(this, ptr); }
C_API bool pool_owns(Pool const* pool, void* ptr)
{
    if (!ptr) {
        return true;
    }
    for (PoolSegment* seg = pool->current; seg; seg = seg->previous) {
        if (seg->pool.owns(ptr)) {
            return true;
        }
    }
    return false;
}

void* Pool::alloc(usize bytes) { return pool_alloc(this, bytes); }
C_API void* pool_alloc(Pool* pool, usize bytes)
{
    usize segment_size = bytes * 2 > min_segment_size ? bytes * 2 : min_segment_size;

    if (!pool->current) {
        pool->current = create_segment(pool->backing_gpa, 0, segment_size, pool->object_size, pool->object_align);
        if (!pool->current) {
            return 0;
        }
    }

    for (PoolSegment* seg = pool->current; seg; seg = seg->next) {
        pool->current = seg;
        void* ptr = seg->pool.alloc(bytes);
        if (ptr) return ptr;
    }

    PoolSegment* seg = create_segment(pool->backing_gpa, pool->current, segment_size, pool->object_size, pool->object_align);
    if (!seg) {
        return 0;
    }
    pool->current->next = seg;
    pool->current = seg;
    return seg->pool.alloc(bytes);
}

void Pool::free(void* ptr, usize size) { return pool_free(this, ptr, size); }
C_API void pool_free(Pool* arena, void* ptr, usize size)
{
    if (!arena->current)
        return;
    VERIFY(arena->owns(ptr));
    VERIFY(arena->owns(((u8*)ptr) + size));
    if (arena->current->pool.owns(ptr)) {
        arena->current->pool.free(ptr, size);
    }
}

static PoolSegment* create_segment(Allocator* a, PoolSegment* previous, usize segment_size, usize object_size, usize object_align)
{
    PoolSegment* segment = (PoolSegment*)memalloc(a, sizeof(PoolSegment) + segment_size, alignof(PoolSegment));
    if (!segment) return 0;

    *segment = (PoolSegment){
        .previous = previous,
        .next = 0,
        .pool = fixed_pool_from_slice(object_size, object_align, segment->buffer, segment_size),
        .segment_size = segment_size,
    };
    return segment;
}

static void* dispatch(Allocator* a, AllocatorEvent event)
{
    Pool* pool = field_base(Pool, allocator, a);
    switch (event.tag) {
    case AllocatorEventTag_Alloc:
        VERIFY(pool->object_align == event.align);
        return pool->alloc(event.byte_count);
    case AllocatorEventTag_Free:
        VERIFY(pool->object_align == event.align);
        pool->free(event.ptr, event.byte_count);
        return nullptr;
    case AllocatorEventTag_Owns:
        return pool->owns(event.ptr) ? (void*)1 : 0;
    }
    return nullptr;
}

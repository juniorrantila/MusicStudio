#include "./Pool.h"

#include "./Allocator.h"
#include "./Verify.h"
#include "./BitSet.h"

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
        .current = nullptr,
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
C_API void* pool_alloc(Pool* pool, usize byte_size)
{
    usize segment_size = byte_size * 2 > min_segment_size ? byte_size * 2 : min_segment_size;

    if (!pool->current) {
        pool->current = create_segment(pool->backing_gpa, 0, segment_size, pool->object_size, pool->object_align);
        if (!pool->current) {
            return nullptr;
        }
    }

    for (PoolSegment* seg = pool->current; seg; seg = seg->next) {
        pool->current = seg;
        void* ptr = seg->pool.alloc(byte_size);
        if (ptr) return memcheck_canary(ptr, byte_size);
    }

    PoolSegment* seg = create_segment(pool->backing_gpa, pool->current, segment_size, pool->object_size, pool->object_align);
    if (!seg) {
        return nullptr;
    }
    pool->current->next = seg;
    pool->current = seg;
    void* ptr = seg->pool.alloc(byte_size);
    if (ptr) memcheck_canary(ptr, byte_size);
    return nullptr;
}

void Pool::free(void* ptr, usize size) { return pool_free(this, ptr, size); }
C_API void pool_free(Pool* pool, void* ptr, usize byte_size)
{
    if (!pool->current)
        return;
    VERIFY(pool->owns(ptr));
    VERIFY(pool->owns(((u8*)ptr) + byte_size));
    if (pool->current->pool.owns(ptr)) {
        pool->current->pool.free(ptr, byte_size);
        memset_canary(ptr, byte_size);
    }
}

static PoolSegment* create_segment(Allocator* a, PoolSegment* previous, usize segment_size, usize object_size, usize object_align)
{
    PoolSegment* segment = (PoolSegment*)memalloc(a, sizeof(PoolSegment) + segment_size, alignof(PoolSegment));
    if (!segment) return nullptr;
    memset_canary(segment->buffer, segment_size);

    *segment = (PoolSegment){
        .previous = previous,
        .next = nullptr,
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
        VERIFYS(usize_popcount(event.align) == 1, "alignment must be a power of two");
        VERIFY(event.align <= pool->object_align);
        return pool->alloc(event.byte_count);
    case AllocatorEventTag_Free:
        VERIFYS(usize_popcount(event.align) == 1, "alignment must be a power of two");
        VERIFY(event.align <= pool->object_align);
        pool->free(event.ptr, event.byte_count);
        return nullptr;
    case AllocatorEventTag_Owns:
        return pool->owns(event.ptr) ? (void*)1 : 0;
    }
    return nullptr;
}

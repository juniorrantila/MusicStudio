#include "./Slab.h"

#include "./Allocator.h"
#include "./Pool.h"
#include "./BitSet.h"
#include "./Verify.h"

static void* dispatch(Allocator*, AllocatorEvent);

static usize size_for_pool(usize pool_index);
static usize align_for_pool(usize pool_index);
static Pool* pool_for_allocation(Slab* slab, usize size, usize align);

Slab Slab::create(Allocator* gpa) { return slab_create(gpa); }
C_API Slab slab_create(Allocator* gpa)
{
    auto slab = (Slab){
        .allocator = make_allocator(dispatch),
        .backing_gpa = gpa,
        .small_pools = {},
        .large_pools = {},
    };
    for (usize i = 0; i < SLAB_POOL_COUNT; i++) {
        slab.small_pools[i] = pool_create(gpa, size_for_pool(i), align_for_pool(i));
    }
    for (usize i = 0; i < SLAB_POOL_COUNT; i++) {
        slab.large_pools[i] = pool_create(gpa, size_for_pool(i), align_for_pool(i));
    }
    return slab;
}

void Slab::destroy() { return slab_destroy(this); }
C_API void slab_destroy(Slab* slab)
{
    for (usize i = SLAB_POOL_COUNT; i-- > 0;) {
        slab->large_pools[i].destroy();
    }
    for (usize i = SLAB_POOL_COUNT; i-- > 0;) {
        slab->small_pools[i].destroy();
    }
    *slab = (Slab){};
}

usize Slab::bytes_used() const { return slab_bytes_used(this); }
C_API usize slab_bytes_used(Slab const* slab)
{
    usize size = 0;
    for (usize i = 0; i < SLAB_POOL_COUNT; i++) {
        size += slab->small_pools[i].bytes_used();
    }
    for (usize i = 0; i < SLAB_POOL_COUNT; i++) {
        size += slab->large_pools[i].bytes_used();
    }
    return size;
}

void Slab::drain() { slab_drain(this); }
C_API void slab_drain(Slab* slab)
{
    for (usize i = 0; i < SLAB_POOL_COUNT; i++) {
        slab->small_pools[i].drain();
    }
    for (usize i = 0; i < SLAB_POOL_COUNT; i++) {
        slab->large_pools[i].drain();
    }
}

bool Slab::owns(void* ptr) const { return slab_owns(this, ptr); }
C_API bool slab_owns(Slab const* slab, void* ptr)
{
    for (usize i = 0; i < SLAB_POOL_COUNT; i++) {
        if (slab->small_pools[i].owns(ptr)) {
            return true;
        }
    }
    for (usize i = 0; i < SLAB_POOL_COUNT; i++) {
        if (slab->large_pools[i].owns(ptr)) {
            return true;
        }
    }
    return false;
}

void* Slab::alloc(usize size, usize align) { return slab_alloc(this, size, align); }
C_API void* slab_alloc(Slab* slab, usize size, usize align)
{
    if (align < 16) align = 16;
    return pool_for_allocation(slab, size, align)->alloc(size);
}

void Slab::free(void* ptr, usize size, usize align) { return slab_free(this, ptr, size, align); }
C_API void slab_free(Slab* slab, void* ptr, usize size, usize align)
{
    pool_for_allocation(slab, size, align)->free(ptr, size);
}

static Pool* pool_for_allocation(Slab* slab, usize size, usize align)
{
    VERIFY(align <= slab_max_alignment());
    VERIFYS(usize_popcount(align) == 1, "alignment must be a power of two");
    for (usize i = 0; i < SLAB_POOL_COUNT; i++) {
        if (align == align_for_pool(i)) {
            usize items = size / size_for_pool(i);
            if (items > 32) {
                return &slab->large_pools[i];
            }
            return &slab->small_pools[i];
        }
    }
    UNREACHABLE();
    return nullptr;
}

static usize size_for_pool(usize pool_index)
{
    return 1 << (pool_index + 4);
}

static usize align_for_pool(usize pool_index)
{
    return 1 << (pool_index + 4);
}

usize slab_max_alignment(void)
{
    return align_for_pool(SLAB_POOL_COUNT);
}

static void* dispatch(Allocator* a, AllocatorEvent event)
{
    Slab* arena = field_base(Slab, allocator, a);
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

#include "./FixedPool.h"

#include "./BitSet.h"
#include "./Verify.h"

#include <string.h>

static void* dispatch(struct Allocator*, AllocatorEvent);
static isize find_sequential_chunk(FixedPool const* pool, usize count);
static usize pool_capacity(FixedPool const*);

static u8* objects_base(FixedPool const*);
static u8* objects_end(FixedPool const*);
static isize object_index(FixedPool const*, u8 const* object);
static u8* object_by_index(FixedPool const*, usize index);

FixedPool FixedPool::from_slice(usize object_size, usize object_align, u8 *memory, usize memory_size) { return fixed_pool_from_slice(object_size, object_align, memory, memory_size); }
C_API FixedPool fixed_pool_from_slice(usize object_size, usize object_align, u8* memory, usize memory_size)
{
    usize size_until_aligned = object_align - ((uptr)memory) % object_align;
    memory_size -= size_until_aligned;
    memory += size_until_aligned;

    usize max_items = memory_size / object_size;
    static_assert(sizeof(*FixedPool::object_allocated) == 1);
    usize objects_allocated_size = (max_items / (8 * 8) + (max_items % 8) != 0);
    max_items -= objects_allocated_size;

    u8* allocated =  memory + (object_size * max_items);
    memset(allocated, 0, objects_allocated_size);
    return (FixedPool){
        .allocator = make_allocator(dispatch),
        .object_size = object_size,
        .object_align = object_align,
        .objects = memory,
        .object_allocated = allocated,
    };
}

void FixedPool::drain() { return fixed_pool_drain(this); }
C_API void fixed_pool_drain(FixedPool* pool)
{
    static_assert(sizeof(*FixedPool::object_allocated) == 1);
    memset(pool->object_allocated, 0, pool_capacity(pool) / 8);
}

usize FixedPool::bytes_used() const { return fixed_pool_bytes_used(this); }
C_API usize fixed_pool_bytes_used(FixedPool const* pool)
{
    usize capacity = pool_capacity(pool);
    usize used_bytes = 0;
    for (usize i = 0; i < capacity; i++) {
        used_bytes += (bit_is_set(pool->object_allocated, i) * pool->object_size);
    }
    return used_bytes;
}

void* FixedPool::alloc(usize byte_size) { return fixed_pool_alloc(this, byte_size); }
C_API void* fixed_pool_alloc(FixedPool* pool, usize byte_size)
{
    VERIFY(byte_size % pool->object_size == 0);
    usize object_count = byte_size / pool->object_size;
    isize chunk = find_sequential_chunk(pool, object_count);
    if (chunk < 0) return nullptr;
    for (usize i = 0; i < object_count; i++) {
        bool was_allocated = bit_set(pool->object_allocated, chunk + i, true);
        VERIFY(!was_allocated);
    }
    return object_by_index(pool, chunk);
}

void FixedPool::free(void* ptr, usize byte_size) { return fixed_pool_free(this, ptr, byte_size); }
C_API void fixed_pool_free(FixedPool* pool, void* ptr, usize byte_size)
{
    VERIFY(byte_size % pool->object_size == 0);
    usize object_count = byte_size / pool->object_size;
    isize index = object_index(pool, (u8*)ptr);
    VERIFY(index >= 0);
    for (usize i = 0; i < object_count; i++) {
        bool was_allocated = bit_set(pool->object_allocated, i, false);
        VERIFY(was_allocated);
    }
}

bool FixedPool::owns(void* ptr) const { return fixed_pool_owns(this, ptr); }
C_API bool fixed_pool_owns(FixedPool const* pool, void* ptr)
{
    return object_index(pool, (u8*)ptr) >= 0;
}

static isize find_sequential_chunk(FixedPool const* pool, usize count)
{
    usize capacity = pool_capacity(pool);

    usize sequential = 0;
    for (usize i = 0; i < capacity; i++) {
        if (bit_is_set(pool->object_allocated, i)) {
            sequential = 0;
            continue;
        }
        sequential += 1;
        if (sequential == count) {
            return ((isize)i) - ((isize)sequential - 1);
        }
    }

    return -1;
}

static usize pool_capacity(FixedPool const* pool)
{
    usize pool_size = objects_end(pool) - objects_base(pool);
    return pool_size / pool->object_size;
}

static u8* object_by_index(FixedPool const* pool, usize index)
{
    return pool->objects + index * pool->object_size;
}

static u8* objects_base(FixedPool const* pool)
{
    return pool->objects;
}

static u8* objects_end(FixedPool const* pool)
{
    return pool->object_allocated - pool->object_size;
}

static isize object_index(FixedPool const* pool, u8 const* object)
{
    if (object < objects_base(pool) || object > objects_end(pool))
        return -1;
    isize index = (isize)(((uptr)object) / ((uptr)objects_end(pool)));
    VERIFY(index >= 0);
    return index;
}

static void* dispatch(struct Allocator* a, AllocatorEvent e)
{
    FixedPool* pool = field_base(FixedPool, allocator, a);
    switch (e.tag) {
    case AllocatorEventTag_Alloc:
        VERIFY(e.align == pool->object_align);
        pool->alloc(e.byte_count);
        return 0;
    case AllocatorEventTag_Free:
        VERIFY(e.align == pool->object_align);
        pool->free(e.ptr, e.byte_count);
        return 0;
    case AllocatorEventTag_Owns:
        return pool->owns(e.ptr) ? (void*)1 : 0;
    }
}

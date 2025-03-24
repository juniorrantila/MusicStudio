#include "./FixedPool.h"

#include "./BitSet.h"
#include "./Verify.h"
#include "./Allocator.h"

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

    usize initial_capacity = memory_size / object_size;
    usize bytes_for_bookeeping = initial_capacity / 8;
    usize capacity = (memory_size - bytes_for_bookeeping) / object_size;
    usize objects_byte_count = capacity * object_size;

    u8* objects = memory;
    memset_canary(objects, objects_byte_count);

    u8* allocated =  memory + objects_byte_count;
    memset(allocated, 0, bytes_for_bookeeping);

    return (FixedPool){
        .allocator = make_allocator(dispatch),
        .capacity = capacity,
        .object_size = object_size,
        .object_align = object_align,
        .objects = objects,
        .object_allocated = allocated,
    };
}

void FixedPool::drain() { return fixed_pool_drain(this); }
C_API void fixed_pool_drain(FixedPool* pool)
{
    static_assert(sizeof(*FixedPool::object_allocated) == 1);
    memset_canary(pool->objects, pool->capacity * pool->object_size);
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

static void alloc_objects(FixedPool* pool, usize start, usize count)
{
    for (usize i = 0; i < count; i++) {
        bool was_allocated = bit_set(pool->object_allocated, start + i, true);
        VERIFYS(!was_allocated, "tried to allocate object that was already allocated");
        memcheck_canary(object_by_index(pool, i), pool->object_size);
    }
}
static void free_objects(FixedPool* pool, usize start, usize count)
{
    for (usize i = 0; i < count; i++) {
        bool was_allocated = bit_set(pool->object_allocated, start + i, false);
        VERIFYS(was_allocated, "tried to free object that was already free");
        memset_canary(object_by_index(pool, i), pool->object_size);
    }
}

void* FixedPool::alloc(usize byte_size) { return fixed_pool_alloc(this, byte_size); }
C_API void* fixed_pool_alloc(FixedPool* pool, usize byte_size)
{
    if (byte_size % pool->object_size != 0) {
        byte_size += pool->object_size - (byte_size % pool->object_size);
    }
    VERIFY(byte_size % pool->object_size == 0);
    usize object_count = byte_size / pool->object_size;
    isize chunk = find_sequential_chunk(pool, object_count);
    if (chunk < 0) return nullptr;
    alloc_objects(pool, chunk, object_count);
    u8* ptr = object_by_index(pool, chunk);
    VERIFY(object_index(pool, ptr) == chunk);
    return memcheck_canary(ptr, byte_size);
}

void FixedPool::free(void* ptr, usize byte_size) { return fixed_pool_free(this, ptr, byte_size); }
C_API void fixed_pool_free(FixedPool* pool, void* ptr, usize byte_size)
{
    if (byte_size % pool->object_size != 0) {
        byte_size += pool->object_size - (byte_size % pool->object_size);
    }
    VERIFY(byte_size % pool->object_size == 0);
    usize object_count = byte_size / pool->object_size;
    isize chunk = object_index(pool, (u8*)ptr);
    VERIFY(chunk >= 0);
    free_objects(pool, chunk, object_count);
    memset_canary(ptr, byte_size);
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
    return pool->objects + pool->capacity * pool->object_size;
}

static isize object_index(FixedPool const* pool, u8 const* object)
{
    if (object < objects_base(pool) || object > objects_end(pool))
        return -1;
    usize index = (((uptr)object) - ((uptr)pool->objects)) / pool->object_size;
    VERIFY(index < pool->capacity);
    return (isize)index;
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

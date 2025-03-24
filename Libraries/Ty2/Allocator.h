#pragma once
#include "./Base.h"

#ifdef __cplusplus
namespace Ty {

template <typename T>
struct View;

template <typename T>
struct Optional;

}
using Ty::View;
using Ty::Optional;
#endif

typedef enum AllocatorEventTag {
    AllocatorEventTag_Alloc,
    AllocatorEventTag_Free,
    AllocatorEventTag_Owns,
} AllocatorEventTag;

typedef struct AllocatorEvent {
    void* ptr;
    usize byte_count;
    usize align;
    AllocatorEventTag tag;
} AllocatorEvent;

typedef struct Allocator {
    void* (*dispatch)(struct Allocator*, AllocatorEvent event);

#ifdef __cplusplus
    void* alloc(usize byte_count, usize align) RETURNS_SIZED_AND_ALIGNED_BY(2, 3);
    void free(void*, usize byte_count, usize align);

    template <typename T>
    T* alloc(usize count) { return (T*)alloc(count * sizeof(T), alignof(T)); }

    template <typename T>
    Optional<View<T>> alloc_many(usize count)
    {
        T* buf = alloc<T>(count);
        if (!buf) return {};
        return View(buf, count);
    }

    template <typename T>
    void free(T* ptr, usize count) { return free(ptr, count * sizeof(T), alignof(T)); }

    template <typename T>
    void free_many(View<T> items)
    {
        free(items.data(), items.size());
    }

    bool owns(void*);

    void* clone(void const* data, usize byte_count, usize align) RETURNS_SIZED_AND_ALIGNED_BY(3, 4);
#endif
} Allocator;

C_API inline Allocator make_allocator(void* (*dispatch)(struct Allocator*, AllocatorEvent))
{
    return (Allocator){
        .dispatch = dispatch,
    };
}

RETURNS_SIZED_AND_ALIGNED_BY(2, 3)
C_API inline void* memalloc(Allocator* a, usize byte_count, usize align)
{
    return a->dispatch(a, (AllocatorEvent){
        .ptr = 0,
        .byte_count = byte_count,
        .align = align,
        .tag = AllocatorEventTag_Alloc
    });
}

C_API inline void memfree(Allocator* a, void* ptr, usize byte_count, usize align)
{
    a->dispatch(a, (AllocatorEvent){
        .ptr = ptr,
        .byte_count = byte_count,
        .align = align,
        .tag = AllocatorEventTag_Free
    });
}

C_API inline bool memowns(Allocator* a, void* ptr)
{
    return a->dispatch(a, (AllocatorEvent){
        .ptr = ptr,
        .byte_count = 0,
        .align = 0,
        .tag = AllocatorEventTag_Owns
    }) != 0;
}

RETURNS_SIZED_AND_ALIGNED_BY(3, 4)
C_API void* memclone(Allocator*, void const* data, usize byte_count, usize align);
C_API void* memclone_zero_extend(Allocator*, void const* data, usize byte_count, usize align, usize extend_bytes);

RETURNS_SIZED_AND_ALIGNED_BY(4, 5)
C_API void* memrealloc(Allocator*, void const* data, usize old_byte_count, usize new_byte_count, usize align);

C_API void* memset_canary(void*, usize);
C_API void* memcheck_canary(void*, usize);

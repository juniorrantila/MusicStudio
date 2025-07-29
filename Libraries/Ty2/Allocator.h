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
    void* alloc(usize byte_count, usize align);
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

    void* clone(void const* data, usize byte_count, usize align);
#endif
} Allocator;

C_INLINE Allocator make_allocator(void* (*dispatch)(struct Allocator*, AllocatorEvent))
{
    return (Allocator){
        .dispatch = dispatch,
    };
}

C_INLINE void* memalloc(Allocator* a, usize byte_count, usize align)
{
    return a->dispatch(a, (AllocatorEvent){
        .ptr = 0,
        .byte_count = byte_count,
        .align = align,
        .tag = AllocatorEventTag_Alloc
    });
}

C_INLINE void memfree(Allocator* a, void* ptr, usize byte_count, usize align)
{
    a->dispatch(a, (AllocatorEvent){
        .ptr = ptr,
        .byte_count = byte_count,
        .align = align,
        .tag = AllocatorEventTag_Free
    });
}

C_INLINE bool memowns(Allocator* a, void* ptr)
{
    return a->dispatch(a, (AllocatorEvent){
        .ptr = ptr,
        .byte_count = 0,
        .align = 0,
        .tag = AllocatorEventTag_Owns
    }) != 0;
}

C_INLINE void* memzero(void* ptr, u64 size)
{
    if (!ptr) return nullptr;
    __builtin_memset(ptr, 0, size);
    return ptr;
}

#if __cplusplus
template <typename T>
T* memzero(T* ptr) { return (T*)memzero(ptr, sizeof(T)); }
#endif

C_API void* memclone(Allocator*, void const* data, usize byte_count, usize align);
C_API void* memclone_zero_extend(Allocator*, void const* data, usize byte_count, usize align, usize extend_bytes);

C_API void* memrealloc(Allocator*, void const* data, usize old_byte_count, usize new_byte_count, usize align);

#pragma once
#include "./Types.h"

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

typedef enum AllocatorEventTag : u32 {
    AllocatorEventTag_Alloc,
    AllocatorEventTag_Free,
    AllocatorEventTag_Owns,
} AllocatorEventTag;

typedef struct AllocatorEvent {
    void* ptr;
    u64 byte_count;
    u64 align;
    AllocatorEventTag tag;
} AllocatorEvent;

typedef struct Allocator {
    void* (*dispatch)(struct Allocator*, AllocatorEvent event);

#ifdef __cplusplus
    void* alloc(u64 byte_count, u64 align);
    void free(void*, u64 byte_count, u64 align);

    template <typename T>
    T* alloc(u64 count) { return (T*)alloc(count * sizeof(T), alignof(T)); }

    template <typename T>
    Optional<View<T>> alloc_many(u64 count)
    {
        T* buf = alloc<T>(count);
        if (!buf) return {};
        return View(buf, count);
    }

    template <typename T>
    void free(T* ptr, u64 count) { return free(ptr, count * sizeof(T), alignof(T)); }

    template <typename T>
    void free_many(View<T> items)
    {
        free(items.data(), items.size());
    }

    bool owns(void*);

    void* clone(void const* data, u64 byte_count, u64 align);

    [[gnu::format(printf, 2, 0)]]
    c_string vfmt(c_string fmt, va_list); 

    [[gnu::format(printf, 2, 3)]]
    c_string fmt(c_string fmt, ...);
#endif
} Allocator;

C_INLINE Allocator allocator_init(void* (*dispatch)(struct Allocator*, AllocatorEvent))
{
    return (Allocator){
        .dispatch = dispatch,
    };
}

C_INLINE void* memalloc(Allocator* a, u64 byte_count, u64 align)
{
    return a->dispatch(a, (AllocatorEvent){
        .ptr = 0,
        .byte_count = byte_count,
        .align = align,
        .tag = AllocatorEventTag_Alloc
    });
}

C_INLINE void memfree(Allocator* a, void* ptr, u64 byte_count, u64 align)
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

#if __has_feature(memory_sanitizer)
C_API void __asan_poison_memory_region(void const volatile *addr, usize size);
C_API void __asan_unpoison_memory_region(void const volatile *addr, usize size);
C_INLINE void mempoison(void const volatile* addr, u64 size) { __asan_poison_memory_region(addr, size); }
C_INLINE void memunpoison(void const volatile* addr, u64 size) { __asan_unpoison_memory_region(addr, size); }
#else
C_INLINE void mempoison(void*, u64) {}
C_INLINE void memunpoison(void*, u64) {}
#endif

C_API [[gnu::format(printf, 2, 0)]]
c_string memvfmt(Allocator*, c_string fmt, va_list);

C_API [[gnu::format(printf, 2, 3)]]
c_string memfmt(Allocator*, c_string fmt, ...);

C_API void* memclone(Allocator*, void const* data, u64 byte_count, u64 align);
C_API void* memclone_zero_extend(Allocator*, void const* data, u64 byte_count, u64 align, u64 extend_bytes);

C_API void* memrealloc(Allocator*, void const* data, u64 old_byte_count, u64 new_byte_count, u64 align);

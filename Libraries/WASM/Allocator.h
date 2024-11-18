#pragma once
#include <Ty/Base.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WASMAllocator {
    void* (*alloc)(void*, usize size, usize align);
    void (*free)(void*, void*, usize size);
} WASMAllocator;
extern WASMAllocator system_allocator;

void* allocator_alloc_impl(WASMAllocator*, usize size, usize align);
#define allocator_alloc(allocator, T) (T*)allocator_alloc_impl(allocator, sizeof(T), alignof(T))
#define allocator_alloc_many(allocator, T, n) (T*)allocator_alloc_impl(allocator, n * sizeof(T), alignof(T))

void allocator_free_impl(WASMAllocator*, void*, usize size);
#define allocator_free(allocator, value) allocator_free_impl(allocator, value, sizeof(*(value)))


#ifdef __cplusplus
}
#endif

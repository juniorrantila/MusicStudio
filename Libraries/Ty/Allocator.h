#pragma once
#include "./Base.h"
#ifdef __cplusplus
#include "./ErrorOr.h"
#include "./Try.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Allocator Allocator;
static inline void* ty_alloc(Allocator* allocator, usize size, usize align);
static inline void ty_free(Allocator* allocator, void* ptr, usize size);

#ifdef __cplusplus
}
#endif

typedef struct Allocator {
    void* (*ialloc)(struct Allocator*, usize size, usize align);
    void (*ifree)(struct Allocator*, void* ptr, usize size);

#ifdef __cplusplus
    ErrorOr<void*> raw_alloc(usize size, usize align,
        c_string func = __builtin_FUNCTION(), 
        c_string file = __builtin_FILE(),
        usize line = __builtin_LINE())
    {
        void* res = ty_alloc(this, size, align);
        if (!res) {
            return Error::from_errno(ENOMEM, func, file, line);
        }
        return res;
    }

    void raw_free(void* data, usize size)
    {
        ty_free(this, data, size);
    }

    template <typename T>
    ErrorOr<T*> alloc(
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        usize line = __builtin_LINE()
    ) {
        return (T*)TRY(raw_alloc(sizeof(T), alignof(T), function, file, line));
    }

    template <typename T>
    ErrorOr<T*> alloc_vla(
        usize array_size,
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        usize line = __builtin_LINE()
    ) {
        return (T*)TRY(raw_alloc(sizeof(T) + sizeof(typename T::VLA) * array_size, alignof(T), function, file, line));
    }

    template <typename T>
    ErrorOr<View<T>> alloc(
        usize size,
        c_string func = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        usize line = __builtin_LINE()
    ) {
        auto* data = (T*)TRY(raw_alloc(size * sizeof(T), alignof(T), func, file, line));
        return View(data, size);
    }

    template <typename T>
    void free(T* data)
    {
        raw_free(data, sizeof(T));
    }

    template <typename T>
    void free_vla(T* data, usize array_size)
    {
        raw_free(data, sizeof(T) + sizeof(typename T::VLA) * array_size);
    }

    template <typename T>
    void free(View<T> view)
    {
        raw_free(view.data(), view.size() * sizeof(T));
    }
#endif
} Allocator;

static inline void* ty_alloc(Allocator* allocator, usize size, usize align) { return allocator->ialloc(allocator, size, align); }
static inline void ty_free(Allocator* allocator, void* ptr, usize size) { allocator->ifree(allocator, ptr, size); }

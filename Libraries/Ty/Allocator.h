#pragma once
#include "./Base.h"
#include "./ErrorOr.h"
#include "./Try.h"

namespace Ty {

struct Allocator {
    virtual ~Allocator() = default;

    virtual ErrorOr<void*> raw_alloc(usize size, usize align,
        c_string func = __builtin_FUNCTION(), 
        c_string file = __builtin_FILE(),
        usize line = __builtin_LINE()) = 0;

    virtual void raw_free(void* data, usize size) = 0;

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
};

}

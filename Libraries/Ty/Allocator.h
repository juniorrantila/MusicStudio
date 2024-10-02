#pragma once
#include <Ty/Base.h>
#include <Ty/ErrorOr.h>
#include <Ty/Try.h>

namespace Ty {

struct Allocator {
    virtual ~Allocator() = default;

    virtual ErrorOr<void*> alloc(usize size, usize align,
        c_string func = __builtin_FUNCTION(), 
        c_string file = __builtin_FILE(),
        usize line = __builtin_LINE()) = 0;

    template <typename T>
    ErrorOr<T*> alloc(
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        usize line = __builtin_LINE()
    ) {
        return (T*)TRY(alloc(sizeof(T), alignof(T), function, file, line));
    }

    template <typename T>
    ErrorOr<View<T>> alloc(
        usize size,
        c_string func = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        usize line = __builtin_LINE()
    ) {
        auto* data = (T*)TRY(alloc(size * sizeof(T), alignof(T), func, file, line));
        return View(data, size);
    }

    virtual void free(void* data, usize size) = 0;

    template <typename T>
    void free(T* data)
    {
        free(data, sizeof(T));
    }

    template <typename T>
    void free(View<T> view)
    {
        free(view.data(), view.size() * sizeof(T));
    }
};

}

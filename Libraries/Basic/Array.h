#pragma once

#include "./Allocator.h"
#include "./Context.h"
#include "./String.h"
#include "./Error.h"

#include <errno.h>

#ifndef __cplusplus

/// requires `array` to have a shape similar to:
/// {
///     Allocator* allocator; // if null, it defaults to temporary_arena()
///     T*  items;
///     u64 count;
///     u64 capacity;
/// }
#define array_expand_if_needed(array)                                                       \
    ({                                                                                      \
        KError __result = kerror_none;                                                      \
        __typeof(array) __a = (array);                                                      \
        if (__a->count >= __a->capacity) {                                                  \
            if (!__a->allocator) __a->allocator = temporary_arena();                                 \
            __typeof(__a->capacity) new_capacity = __a->capacity * 2;                       \
            if (new_capacity < 8) new_capacity = 8;                                         \
            u64 old_size = __a->count * sizeof(__a->items[0]);                              \
            u64 size = new_capacity * sizeof(__a->items[0]);                                \
            u64 align = alignof(__typeof(__a->items[0]));                                   \
            void* items = memrealloc(__a->allocator, __a->items, old_size, size, align);    \
            if (items) {                                                                    \
                __a->capacity = new_capacity;                                               \
                __a->items = (__typeof(__a->items))items;                                   \
            } else {                                                                        \
                __result = kerror_unix(ENOMEM);                                             \
            }                                                                               \
        }                                                                                   \
        __result;                                                                           \
    })

/// requires `array` to have a shape similar to:
/// {
///     Allocator* allocator; // if null, it defaults to temporary_arena()
///     T*  items;
///     u64 count;
///     u64 capacity;
/// }
#define array_push(array, ...)                                  \
    ({                                                          \
        KError __result = kerror_none;                          \
        __typeof(array) __array = (array);                      \
        __result = array_expand_if_needed(__array);             \
        if (__result.ok) {                                      \
            __array->items[__array->count++] = (__VA_ARGS__);   \
        }                                                       \
        __result;                                               \
    })

/// requires `array` to have a shape similar to:
/// {
///     u64 count;
///     T items[SIZE];
/// }
#define static_push(array, ...)                                 \
    ({                                                          \
        KError __result = kerror_none;                          \
        __typeof(array) __array = (array);                      \
        if (__array->count < ARRAY_SIZE(__array->items)) {      \
            __array->items[__array->count++] = (__VA_ARGS__);   \
        } else {                                                \
            __result = kerror_unix(ENOMEM);                     \
        }                                                       \
        __result;                                               \
    })

/// requires `array` to have a shape similar to:
/// {
///     u64 count;
///     T items[SIZE];
/// }
#define static_size_left(array, ...) (ARRAY_SIZE((array)->items) - (array)->count)

#else

template <typename Array>
concept IsArrayLike = requires(Array a) {
    a.items;
    a.count;
    a.capacity;
};

template <typename Array>
concept IsStaticArrayLike = requires(Array a) {
    a.items;
    a.count;
};

template <typename Array>
concept HasAllocator = requires(Array a) {
    a.allocator;
};

template <typename Array, typename U>
concept IndexIsAssignable = requires(Array a, U value) {
    a.items[0] = value;
};

/// requires `array` to have a shape similar to:
/// {
///     Allocator* allocator; // if null, it defaults to temporary_arena()
///     T*  items;
///     u64 count;
///     u64 capacity;
/// }
template <typename Array>
    requires IsArrayLike<Array>
KError array_expand_if_needed(Array* array)
{
    using T = __typeof(array->items[0]);
    KError result = kerror_none;
    if (array->count >= array->capacity) {
        Allocator* allocator = nullptr;
        if constexpr (HasAllocator<Array>) {
            if (!array->allocator) array->allocator = temporary_arena();
            allocator = array->allocator;
        } else {
            allocator = temporary_arena();
        }
        auto new_capacity = array->capacity * 2;
        if (new_capacity < 8) new_capacity = 8;
        u64 old_size = array->count * sizeof(T);
        u64 size = new_capacity * sizeof(T);
        u64 align = alignof(T);
        void* items = memrealloc(allocator, array->items, old_size, size, align);
        if (items) {
            ty_memcpy(items, array->items, old_size);
            array->capacity = new_capacity;
            array->items = (T*)items;
        } else {
            result = kerror_unix(ENOMEM);
        }
    }
    return result;
}

/// requires `array` to have a shape similar to:
/// {
///     Allocator* allocator; // if null, it defaults to temporary_arena()
///     T*  items;
///     u64 count;
///     u64 capacity;
/// }
template <typename Array, typename U>
    requires IsArrayLike<Array> and IndexIsAssignable<Array, U>
KError array_push(Array* array, U item)
{
    KError result = kerror_none;
    result = array_expand_if_needed(array);
    if (result.ok) array->items[array->count++] = item;
    return result;
}


/// requires `array` to have a shape similar to:
/// {
///     u64 count;
///     T items[SIZE];
/// }
template <typename Array, typename U>
    requires (IsStaticArrayLike<Array> and IndexIsAssignable<Array, U>)
KError static_push(Array* array, U item)
{
    if (array->count + 1 >= ARRAY_SIZE(array->items))
        return kerror_unix(ENOMEM);
    array->items[array->count++] = item;
    return kerror_none;
}

/// requires `array` to have a shape similar to:
/// {
///     Allocator* allocator; // if null, it defaults to temporary_arena()
///     T*  items;
///     u64 count;
///     u64 capacity;
/// }
template <typename Array>
    requires IsStaticArrayLike<Array>
constexpr auto array_iter(Array array)
{
    using T = __typeof(array.items[0]);
    struct Iterator {
        Array array;

        T* begin() { return array.items; };
        T* end() { return array.items + array.count; };

        T* begin() const { return array.items; };
        T* end() const { return array.items + array.count; };
    };

    return Iterator{array};
}

template <typename T>
struct Index {
    u8 slice_id : 4 = 0;
    i64 index : 60 = 0;
};
static_assert(sizeof(Index<void>) == 8);

template <typename T>
struct Slice {
    static inline _Atomic u32 serial_number;

    T* items = nullptr;
    u8 slice_id : 4 = ++serial_number & 0x0F;
    i64 count : 60 = 0;

    constexpr Slice() = default;

    template <i64 Count>
    constexpr Slice(T (&items)[Count])
        : items(items)
        , count(Count)
    {
    }

    constexpr Slice(T* items, i64 count)
        : items(items)
        , count(count)
    {
    }

    operator Slice<T const>() const { return Slice<T const>(items, count); }

    Index<T> index_of(i64 i) const
    {
        return {
            .slice_id = slice_id,
            .index = i,
        };
    }

    Index<T> adopt_index(Index<T> i) const
    {
        return {
            .slice_id = slice_id,
            .index = i.index,
        };
    }

    constexpr T& at(i64 index)
    {
        DEBUG_ASSERT(index < count);
        DEBUG_ASSERT(items != nullptr);
        return items[index];
    }

    constexpr T const& at(i64 index) const
    {
        DEBUG_ASSERT(index < count);
        DEBUG_ASSERT(items != nullptr);
        return items[index];
    }

    constexpr T& at(Index<T> index)
    {
        DEBUG_ASSERT(index.index < count);
        DEBUG_ASSERT(items != nullptr);
        DEBUG_ASSERT(index.slice_id == slice_id);
        return items[index.index];
    }

    constexpr T const& at(Index<T> index) const
    {
        DEBUG_ASSERT(index.index < count);
        DEBUG_ASSERT(items != nullptr);
        DEBUG_ASSERT(index.slice_id == slice_id);
        return items[index.index];
    }

    constexpr T& operator[](i64 index) { return at(index); }
    constexpr T const& operator[](i64 index) const { return at(index); }
    constexpr T& operator[](Index<T> index) { return at(index); }
    constexpr T const& operator[](Index<T> index) const { return at(index); }

    constexpr T const* begin() const { return items; }
    constexpr T const* end() const { return items + count; }
    constexpr T* begin() { return items; }
    constexpr T* end() { return items + count; }
};

template <typename T>
struct TemporaryArray : public Slice<T> {
    i64 capacity = 0;

    ErrorOr<Index<T>> push(T value)
        requires IndexIsAssignable<Slice<T>, T>
    {
        if (auto error = array_push(this, value))
            return error;
        return this->index_of(this->count - 1);
    }
};

template <typename T>
struct Array : public Slice<T> {
    i64 capacity = 0;
    Allocator* allocator = nullptr;

    ErrorOr<Index<T>> push(T value)
        requires IndexIsAssignable<Slice<T>, T>
    {
        if (auto error = array_push(this, value))
            return error;
        return this->index_of(this->count - 1);
    }
};

#endif

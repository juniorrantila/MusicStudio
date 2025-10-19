#pragma once
#include "./Types.h"
#include "./Verify.h"
#include "./Allocator.h"
#include "./Maybe.h"

VALIDATE_IS_CPP();

template <typename T>
struct Span {
    using Type = T;

    T* items { nullptr };
    u64 count { 0 };

    constexpr Span() = default;

    constexpr Span(T* items, u64 count)
        : items(items)
        , count(count)
    {
    }

    template <u64 Count>
    constexpr Span(T const (&items)[Count])
        : items(items)
        , count(Count)
    {
    }

    operator Span<const T>() const { return Span<T const>(items, count); }

    static Maybe<Span> alloc(Allocator* a, u64 count)
    {
        T* items = (T*)memalloc(a, count * sizeof(T), alignof(T));
        if (!items)
            return {};
        return (Span){
            .items = items,
            .count = count,
        };
    }

    void free(Allocator* a) const
    {
        memfree(a, items, sizeof(T) * count, alignof(T));
    }

    Span zero() const
    {
        memzero(items, count * sizeof(T));
        return *this;
    }

    T& operator[](u64 index)
    {
        VERIFY(index < count);
        return items[index];
    }

    T const& operator[](u64 index) const
    {
        VERIFY(index < count);
        return items[index];
    }

    T* begin() { return items; }
    T* end() { return items + count; }

    T const* begin() const { return items; }
    T const* end() const { return items + count; }
};

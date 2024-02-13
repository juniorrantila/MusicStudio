#pragma once
#include "Base.h"
#include "Id.h"
#include "Optional.h"

namespace Ty {

template <typename T, u16 Capacity>
struct StaticVector {
    T m_data[Capacity] {};
    u16 m_size { 0 };

    StaticVector() = default;

    constexpr u16 capacity() { return Capacity; }
    constexpr u16 size() { return m_size; }

    constexpr T* begin() { return m_data; }
    constexpr T const* begin() const { return m_data; }

    constexpr T* end() { return m_data[m_size]; }
    constexpr T const* end() const { return m_data[m_size]; }

    constexpr Optional<SmallId<T>> append(T value)
    {
        if (m_size >= capacity())
            return {};
        m_data[m_size] = value;
        auto id = SmallId<T>(m_size);
        m_size++;
        return id;
    }

    constexpr Optional<SmallId<T>> find_or_append(T value)
    {
        auto id = find(value);
        if (id.has_value())
            return id.release_value();
        return append(value);
    }

    constexpr Optional<SmallId<T>> find(T value)
    {
        for (u16 i = 0; i < m_size; i++) {
            if (m_data[i] == value)
                return SmallId<T>(i);
        }
        return {};
    }

    constexpr T& operator[](SmallId<T> id)
    {
        return m_data[id.raw()];
    }

    constexpr T const& operator[](SmallId<T> id) const
    {
        return m_data[id.raw()];
    }

    constexpr T& operator[](u16 index) { return m_data[index]; }

    constexpr T const& operator[](u16 index) const
    {
        return m_data[index];
    }
};

}

using Ty::StaticVector;

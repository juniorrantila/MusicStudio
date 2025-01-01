#pragma once
#include "./Forward.h"

#include "./Base.h"
#include "./ErrorOr.h"
#include "./FormatCounter.h"
#include "./Memory.h"
#include "./Try.h"
#include "./View.h"

namespace Ty {

template <typename T>
struct Buffer {

    static constexpr ErrorOr<Buffer> create(u64 capacity)
    {
        return Buffer {
            (T*)TRY(allocate_memory(sizeof(T) * capacity)),
            capacity,
        };
    }

    constexpr Buffer() = default;

    constexpr Buffer(Buffer&& other)
        : m_data(other.m_data)
        , m_capacity(other.m_capacity)
    {
        other.invalidate();
    }

    constexpr ~Buffer()
    {
        if (is_valid()) {
            free_memory(m_data);
            invalidate();
        }
    }

    constexpr Buffer& operator=(Buffer&& other)
    {
        if (this == &other) {
            return *this;
        }
        this->~Buffer();

        m_data = other.m_data;
        m_capacity = other.m_capacity;
        other.invalidate();

        return *this;
    }

    T& operator[](usize i)
    {
        VERIFY(i < capacity());
        return m_data[i];
    }

    T const& operator[](usize i) const
    {
        VERIFY(i < capacity());
        return m_data[i];
    }

    Optional<T> at(usize i) const
    {
        if (i >= capacity())
            return {};
        return m_data[i];
    }

    constexpr T* mutable_data() { return m_data; }
    constexpr T const* data() const { return m_data; }
    constexpr usize capacity() const { return m_capacity; }

    constexpr T* begin() { return m_data; }
    constexpr T* end() { return &m_data[m_capacity]; }

    constexpr T const* begin() const { return m_data; }
    constexpr T const* end() const { return &m_data[m_capacity]; }

    constexpr View<T const> view() const { return { m_data, m_capacity }; }

    constexpr Bytes as_bytes() const { return { m_data, byte_size() }; }

    constexpr usize byte_size() const { return m_capacity * sizeof(T); }

private:
    constexpr Buffer(T* data, usize capacity)
        : m_data(data)
        , m_capacity(capacity)
    {
    }

    constexpr bool is_valid() const { return m_data != nullptr; }
    constexpr void invalidate() { m_data = nullptr; }

    T* m_data { nullptr };
    usize m_capacity { 0 };
};

}

#pragma once
#include "Base.h"
#include "Concepts.h"
#include "ErrorOr.h"
#include "FormatCounter.h"
#include "Formatter.h"
#include "Forward.h"
#include "Memory.h"
#include "Traits.h"
#include "Try.h"
#include "Vector.h"

namespace Ty {

struct ByteBuffer {

    static constexpr ErrorOr<ByteBuffer> create_saturated(
        u32 capacity)
    {
        return ByteBuffer {
            (u8*)TRY(allocate_memory(capacity)),
            capacity,
        };
    }

    static constexpr ErrorOr<ByteBuffer> create(
        u32 capacity = inline_capacity)
    {
        if (capacity > inline_capacity)
            return create_saturated(capacity);
        return ByteBuffer();
    }

    template <typename... Args>
    static constexpr ErrorOr<ByteBuffer> create_saturated_fill(
        Args... args) requires(sizeof...(args) > 1)
    {
        auto capacity = TRY(FormatCounter::count(args...)) + 1;
        auto buffer = TRY(create_saturated(capacity));
        TRY(buffer.write(args...));
        return buffer;
    }

    template <typename... Args>
    static constexpr ErrorOr<ByteBuffer> create_fill(
        Args... args) requires(sizeof...(args) > 1)
    {
        auto capacity = TRY(FormatCounter::count(args...)) + 1;
        auto buffer = TRY(create(capacity));
        TRY(buffer.write(args...));
        return buffer;
    }

    template <typename T>
    static constexpr ErrorOr<ByteBuffer> create_fill(T value)
    {
        auto capacity = TRY(FormatCounter::count(value)) + 1;
        auto buffer = TRY(create(capacity));
        TRY(buffer.write(value));
        return buffer;
    }

    constexpr ByteBuffer()
        : m_data(m_storage)
        , m_capacity(inline_capacity)
    {
    }

    constexpr ByteBuffer(ByteBuffer&& other)
        : m_data(other.m_data)
        , m_size(other.m_size)
        , m_capacity(other.m_capacity)
    {
        if (!other.is_saturated()) {
            __builtin_memcpy(m_storage, other.m_storage,
                inline_capacity);
            m_data = m_storage;
        }
        other.invalidate();
    }

    constexpr ~ByteBuffer()
    {
        if (is_valid()) {
            if (is_saturated())
                free_memory(m_data);
            invalidate();
        }
    }

    constexpr ByteBuffer& operator=(ByteBuffer&& other)
    {
        this->~ByteBuffer();

        m_data = other.m_data;
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        if (!other.is_saturated()) {
            __builtin_memcpy(m_storage, other.m_storage,
                inline_capacity);
            m_data = m_storage;
        }
        other.invalidate();

        return *this;
    }

    template <typename... Args>
    constexpr ErrorOr<u32> write(Args... args)
        requires(sizeof...(Args) > 1)
    {
        constexpr auto args_size = sizeof...(Args);
        ErrorOr<u32> results[args_size] = {
            write(args)...,
        };
        u32 written = 0;
        for (u32 i = 0; i < args_size; i++)
            written += TRY(results[i]);
        return written;
    }

    constexpr ErrorOr<u32> write(StringView string)
    {
        TRY(expand_if_needed_for_write(string.size));
        auto size = string.unchecked_copy_to((char*)&m_data[m_size]);
        m_size += size;
        return size;
    }

    constexpr ErrorOr<u32> write(View<u8 const> bytes)
    {
        TRY(expand_if_needed_for_write(bytes.size()));
        __builtin_memcpy(m_data, bytes.data(), bytes.size());
        m_size += bytes.size();
        return bytes.size();
    }

    constexpr void clear() { m_size = 0; }

    constexpr u8* mutable_data() { return m_data; }
    constexpr u8 const* data() const { return m_data; }
    constexpr u32 size() const { return m_size; }
    constexpr u32 capacity() const { return m_capacity; }
    constexpr u32 size_left() const { return m_capacity - m_size; }

    constexpr u8* begin() { return m_data; }
    constexpr u8* end() { return &m_data[m_size]; }

    constexpr u8 const* begin() const { return m_data; }
    constexpr u8 const* end() const { return &m_data[m_size]; }

    constexpr View<u8 const> view() const { return { m_data, m_size }; }

    ErrorOr<void> expand_if_needed_for_write(u32 size)
    {
        if (m_size + size >= m_capacity)
            TRY(expand_by(size));
        return {};
    }

    ErrorOr<void> expand_by(u32 size)
    {
        auto new_capacity = m_size + size;
        auto* new_data = (u8*)TRY(allocate_memory(new_capacity));
        __builtin_memcpy(new_data, data(), m_size);
        if (is_saturated())
            free_memory(m_data);
        m_data = new_data;
        m_capacity = new_capacity;
        return {};
    }

private:
    static constexpr auto max_chars_in_u64 = 20;
    static constexpr auto inline_capacity = 1024;

    constexpr ByteBuffer(u8* data, u32 capacity)
        : m_data(data)
        , m_size(0)
        , m_capacity(capacity)
    {
    }

    constexpr bool is_valid() const { return m_data != nullptr; }
    constexpr void invalidate() { m_data = nullptr; }

    constexpr bool is_saturated() const
    {
        return m_data != m_storage;
    }

    u8 m_storage[inline_capacity];
    u8* m_data { nullptr };
    u32 m_size { 0 };
    u32 m_capacity { 0 };
};

template <>
struct Formatter<ByteBuffer> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to,
        ByteBuffer const& buffer)
    {
        return TRY(to.write(buffer.view()));
    }
};

}

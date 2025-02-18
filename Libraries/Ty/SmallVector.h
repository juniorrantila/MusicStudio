#pragma once
#include "./Base.h"
#include "./ErrorOr.h"
#include "./Id.h"
#include "./Move.h"
#include "./ReverseIterator.h"
#include "./View.h"

namespace Ty {

template <typename T, u32 capacity = 16>
struct SmallVector {
    constexpr SmallVector() = default;

    SmallVector& operator=(SmallVector const&) = delete;
    constexpr SmallVector& operator=(SmallVector&& other)
    {
        if (this == &other)
            return *this;
        m_size = other.m_size;
        __builtin_memcpy(m_data, other.m_data, other.m_size);
        other.invalidate();
        return *this;
    }

    constexpr SmallVector(SmallVector&& other)
        : m_size(other.m_size)
    {
        destroy_elements();
        for (u32 i = 0; i < m_size; i++) {
            new (slot(i)) T(move(*other.slot(i)));
        }
        other.invalidate();
    }

    constexpr ~SmallVector()
    {
        if (is_valid()) {
            destroy_elements();
            invalidate();
        }
    }

    constexpr ErrorOr<Id<T>> append(T&& value) requires(
        !is_trivially_copyable<T>)
    {
        if (!is_valid())
            return Error::from_string_literal(
                "SmallVector filled, you need to increase the "
                "capacity");
        new (current_slot()) T(move(value));
        return Id<T>(m_size++);
    }

    constexpr ErrorOr<Id<T>> append(T value) requires(
        is_trivially_copyable<T>)
    {
        if (!is_valid())
            return Error::from_string_literal(
                "SmallVector filled, you need to increase the "
                "capacity");
        new (current_slot()) T(value);
        return Id<T>(m_size++);
    }

    constexpr Id<T> must_append(T&& value) requires(
        !is_trivially_copyable<T>)
    {
        return MUST(append(move(value)));
    }

    constexpr Id<T> must_append(T value) requires(
        is_trivially_copyable<T>)
    {
        return MUST(append(value));
    }

    constexpr View<T> view() { return { data(), size()}; }
    constexpr View<T const> view() const
    {
        return { data(), size() };
    }

    constexpr T const& at(u32 index) const { return *slot(index); }

    constexpr T const& at(Id<T> id) const { return at(id.raw()); }

    constexpr T const& operator[](u32 index) const
    {
        return at(index);
    }

    constexpr T const& operator[](Id<T> id) const { return at(id); }

    constexpr T& operator[](u32 index) { return *slot(index); }

    constexpr T& operator[](Id<T> id) { return *slot(id.raw()); }

    constexpr T* begin() { return slot(0); }
    constexpr T* end() { return slot(m_size); }

    constexpr T const* begin() const { return slot(0); }
    constexpr T const* end() const { return slot(m_size); }

    ReverseIterator<T const> in_reverse() const
    {
        return { begin(), end() };
    }

    ReverseIterator<T> in_reverse() { return { begin(), end() }; }

    constexpr T* data() { return slot(0); }
    constexpr T const* data() const { return slot(0); }
    constexpr u32 size() const { return m_size; }

    constexpr bool is_empty() const { return m_size == 0; }

    constexpr bool is_valid() const { return m_size != capacity; }

    constexpr void clear()
    {
        destroy_elements();
        m_size = 0;
    }

    Optional<T> pop()
    {
        if (m_size == 0) {
            return Optional<T>{};
        }
        m_size -= 1;
        return move(at(m_size));
    }

    void shrink(u32 count)
    {
        for (u32 i = 0; i < count; i++) {
            (void)pop();
        }
    }

private:
    constexpr void destroy_elements() const
        requires(!is_trivially_destructible<T>)
    {
        for (u32 i = 0; i < m_size; i++)
            slot(i)->~T();
    }

    constexpr void destroy_elements() const
        requires(is_trivially_destructible<T>)
    {
    }

    constexpr T* slot(u32 index) { return &((T*)m_data)[index]; }

    constexpr T const* slot(u32 index) const
    {
        return &((T const*)m_data)[index];
    }

    constexpr T* current_slot() { return slot(m_size); }

    constexpr T const* current_slot() const { return slot(m_size); }

    constexpr void invalidate() { m_size = capacity; }

    alignas(T) u8 m_data[sizeof(T) * capacity] {};
    u32 m_size { 0 };
};

}

using namespace Ty;

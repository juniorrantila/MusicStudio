#pragma once
#include "Base.h"
#include "Id.h"
#include "Optional.h"
#include "Try.h"
#include "ErrorOr.h"

namespace Ty {

template <typename T, u16 Size>
struct CircularQueue {
    constexpr Optional<Id<T>> try_push(T& value)
    {
        if ((m_head + 1) % size() == m_tail)
            return {};
        slot(m_head) = move(value);
        auto id = Id<T>(m_head++);
        m_head %= size();
        return id;
    }

    constexpr Optional<T> try_pop()
    {
        if (m_tail == m_head)
            return {};
        auto& value = slot(m_tail++);
        m_tail %= size();
        return move(value);
    }

    constexpr T& operator[](Id<T> id)
    {
        return slot(id.raw());
    }

    constexpr T const& operator[](Id<T> id) const
    {
        return slot(id.raw());
    }

private:

    constexpr static u16 size() { return Size; }

    constexpr T& slot(u32 index)
    {
        return reinterpret_cast<T*>(m_storage)[index];
    }

    constexpr T const& slot(u32 index) const
    {
        return reinterpret_cast<T const*>(m_storage)[index];
    }

    alignas(T) u8 m_storage[sizeof(T) * size()];
    u16 m_head { 0 };
    u16 m_tail { 0 };
};

}

#pragma once

#include "./Optional.h"

namespace Ty {

template <typename T>
struct Ref {
    constexpr explicit Ref(T& value)
        : m_ptr(&value)
    {
    }

    static constexpr Optional<Ref> from_ptr(T* value)
    {
        if (!value) {
            return {};
        }
        return Ref(value);
    }

    operator T*()
    {
        VERIFY(m_ptr);
        return m_ptr;
    }

    operator T const*() const
    {
        VERIFY(m_ptr);
        return m_ptr;
    }

    T* operator->()
    {
        VERIFY(m_ptr);
        return m_ptr;
    }

    T const* operator->() const
    {
        VERIFY(m_ptr);
        return m_ptr;
    }

    T& operator*()
    {
        VERIFY(m_ptr);
        return *m_ptr;
    }

    T const& operator*() const
    {
        VERIFY(m_ptr);
        return *m_ptr;
    }

private:
    constexpr Ref(T* ptr)
        : m_ptr(ptr)
    {
    }

    T* m_ptr { nullptr };
};

}

using Ty::Ref;

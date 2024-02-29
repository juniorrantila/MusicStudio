#pragma once
#include "./Base.h"
#include "./ErrorOr.h"

namespace Ty {

template <typename T>
struct RefPtr {

    static ErrorOr<RefPtr> create(T data)
    {
        return RefPtr(new Storage(0, move(data)));
    }
    ~RefPtr()
    {
        if (m_outline) {
            m_outline->reference_count--;
            if (m_outline->reference_count == 0) {
                delete m_outline;
            }
        }
    }

    RefPtr(RefPtr const& other)
        : m_outline(other.m_outline)
    {
        m_outline->reference_count++;
    }

    RefPtr& operator=(RefPtr const& other)
    {
        if (this == &other)
            return *this;
        m_outline = other.m_outline;
        m_outline->reference_count++;
        return *this;
    }
    
    RefPtr(RefPtr&& other)
        : m_outline(other.m_outline)
    {
        other.m_outline = nullptr;
    }
    RefPtr& operator=(RefPtr&& other)
    {
        if (this == &other)
            return *this;
        m_outline = other.m_outline;
        other.m_outline = nullptr;
    }

    T* operator->() { return &m_outline->data; }
    T const* operator->() const { return &m_outline->data; }
    
private:
    struct Storage {
        usize reference_count { 0 };
        T data {};
    };
    RefPtr(Storage* storage)
        : m_outline(storage)
    {
        m_outline->reference_count++;
    }

    Storage* m_outline { 0 };
};

}

using Ty::RefPtr;

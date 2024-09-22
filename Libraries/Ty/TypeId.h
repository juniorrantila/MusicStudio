#pragma once
#include "./Base.h"
#include "./Verify.h"

namespace Ty {

namespace Internal {

inline u16 next_id()
{
    static u16 id = 0;
    VERIFY(id != 0xFFFF);
    return id++;
}

template <typename T>
struct TypeTrick {
    static u16 eval()
    {
        if (id != 0xFFFF)
            return id;
        return id = next_id();
    }

private:
    static inline u16 id { 0xFFFF };
};

}

struct TypeId {
    explicit constexpr TypeId(u16 raw)
        : m_raw(raw)
    {
    }

    constexpr bool operator==(TypeId other) const { return m_raw == other.m_raw; }

    u16 raw() const { return m_raw; }

private:
    u16 m_raw { 0 };
};

template <typename T>
static inline TypeId type_id()
{
    return TypeId(Internal::TypeTrick<T>::eval());
}

}

using Ty::type_id;

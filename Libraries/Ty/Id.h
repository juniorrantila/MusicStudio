#pragma once
#include "Base.h"

namespace Ty {

template <typename T>
struct Id {
    struct InvalidToken { };
    static constexpr InvalidToken Invalid;
    constexpr Id(InvalidToken) { }
    constexpr bool operator==(InvalidToken) const
    {
        return !is_valid();
    }

    constexpr Id() = default;

    constexpr explicit Id(u32 raw_id)
        : m_raw(raw_id)
    {
    }

    constexpr u32 raw() const { return m_raw; }

    static constexpr Id<T> invalid()
    {
        return Id<T> { 0xFFFFFFFF };
    }
    constexpr bool is_valid() const
    {
        return m_raw != invalid().raw();
    }

    constexpr bool operator==(Id other) const
    {
        return m_raw == other.m_raw;
    }

private:
    u32 m_raw { invalid().raw() };
};

template <typename T>
struct SmallId {
    struct InvalidToken { };
    static constexpr auto Invalid = InvalidToken();
    constexpr SmallId(InvalidToken) { }
    constexpr bool operator==(InvalidToken) const
    {
        return !is_valid();
    }

    constexpr SmallId() = default;

    constexpr explicit SmallId(u16 raw_id)
        : m_raw(raw_id)
    {
    }

    constexpr u16 raw() const { return m_raw; }

    static constexpr SmallId<T> invalid()
    {
        return SmallId<T> { 0xFFFF };
    }

    constexpr bool is_valid() const
    {
        return m_raw != invalid().raw();
    }

    constexpr bool operator==(SmallId other) const
    {
        return m_raw == other.m_raw;
    }

private:
    u16 m_raw { invalid().raw() };
};

}

using namespace Ty;

#pragma once
#include "./Types.h"

VALIDATE_IS_CPP();

template <typename T>
struct Maybe {
    using Type = T;

    constexpr Maybe(T value)
        : m_value(value)
        , m_has_value(true)
    {
    }

    constexpr Maybe() = default;

    bool has_value() const { return m_has_value; }
    void release_error() const {  }
    T release_value() const { VERIFY(has_value()); return *storage(); }
    T value() const { VERIFY(has_value()); return *storage(); }

    constexpr T* operator->() { VERIFY(has_value()); return storage(); }
    constexpr T const* operator->() const { VERIFY(has_value()); return storage(); }

    constexpr T const& operator*() { VERIFY(has_value()); return value(); }

    explicit constexpr operator bool() const { return has_value(); }

private:
    constexpr T* storage() { return &m_value; }
    constexpr T const* storage() const { return &m_value; }

    T m_value;
    bool m_has_value { false };
};

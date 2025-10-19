#pragma once
#ifndef __cplusplus
#error "this file only works in C++ mode"
#endif

namespace Ty { struct Error; }
template <typename T, typename E = Ty::Error>
struct Result {
    constexpr Result(T value)
        : m_value(value)
        , m_has_value(true)
    {
    }

    constexpr Result(E value)
        : m_error(value)
        , m_has_value(false)
    {
    }

    constexpr bool is_error() const { return !has_value(); }
    constexpr bool has_value() const { return m_has_value; }
    constexpr E release_error() const { VERIFY(is_error()); return m_error; }
    constexpr T release_value() const { VERIFY(has_value()); return m_value; }
    constexpr E const& error() const { VERIFY(is_error()); return m_error; }
    constexpr T const& value() const { VERIFY(has_value()); return m_value; }

    constexpr T* operator->() { VERIFY(has_value()); return &value(); }
    constexpr T const* operator->() const { VERIFY(has_value()); return &value(); }

    constexpr T const& operator*() { VERIFY(has_value()); return value(); }

    explicit constexpr operator bool() const { return has_value(); }

private:
    constexpr T* storage() { return &m_value; }
    constexpr T const* storage() const { return &m_value; }

    union {
        T m_value;
        E m_error;
    };
    bool m_has_value { false };
};

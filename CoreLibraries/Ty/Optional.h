#pragma once
#include "./Base.h"
#include "./Forward.h"
#include "./Move.h"
#include "./New.h"
#include "./Traits.h"
#include "./Verify.h"

namespace Ty {

template <typename T>
struct [[nodiscard]] Optional {

    constexpr Optional() = default;

    constexpr Optional(T value) requires is_trivially_copyable<T>
        : m_has_value(true)
    {
        new (storage()) T(value);
    }

    constexpr Optional(T&& value) requires(
        !is_trivially_copyable<T>)
        : m_has_value(true)
    {
        new (storage()) T(move(value));
    }

    constexpr Optional(Optional&& other)
        : m_has_value(other.has_value())
    {
        if (m_has_value) {
            new (storage()) T(other.release_value());
        } else {
            new (storage()) T();
        }
    }

    constexpr ~Optional() { clear_if_needed(); }

    constexpr Optional& operator=(
        T value) requires is_trivially_copyable<T>
    {
        clear_if_needed();
        new (storage()) T(value);
        m_has_value = true;
        return *this;
    }

    constexpr Optional& operator=(T&& value) requires(
        !is_trivially_copyable<T>)
    {
        clear_if_needed();
        new (storage()) T(move(value));
        m_has_value = true;
        return *this;
    }

    constexpr Optional& operator=(
        Optional value) requires is_trivially_copyable<T>
    {
        clear_if_needed();
        new (this) Optional(value);
        return *this;
    }

    constexpr Optional& operator=(Optional&& value) requires(
        !is_trivially_copyable<T>)
    {
        clear_if_needed();
        new (this) Optional(move(value));
        m_has_value = true;
        return *this;
    }

    template <typename F>
    constexpr decltype(auto) or_throw(F error_callback)
    {
        using Return = ErrorOr<T, decltype(error_callback())>;
        if (has_value())
            return Return(release_value());
        return Return(error_callback());
    }

    template <typename F>
    constexpr decltype(auto) or_else(F callback)
        requires IsCallableWithArguments<F, T>
    {
        if (!has_value())
            return callback();
        using Return = decltype(callback());
        return Return(release_value());
    }

    template <typename U>
    constexpr T or_else(U value)
        requires (!IsFunction<U> and IsConvertible<U, T>)
    {
        if (!has_value())
            return T(value);
        return release_value();
    }

    constexpr T value_or(T otherwise)
    {
        if (has_value())
            return value();
        return otherwise;
    }

    constexpr T& value() { return *storage(); }
    constexpr T const& value() const { return *storage(); }

    constexpr T release_value()
    {
        m_has_value = false;
        return move(value());
    }

    constexpr bool has_value() const { return m_has_value; }

    T unwrap()
    {
        VERIFY(has_value());
        return release_value();
    }

    constexpr T* operator->() { VERIFY(has_value()); return storage(); }
    constexpr T const* operator->() const { VERIFY(has_value()); return storage(); }

    constexpr T const& operator*() { VERIFY(has_value()); return value(); }

    explicit constexpr operator bool() const { return has_value(); }

    constexpr bool operator==(T const& other) const
    {
        return has_value() && value() == other;
    }

private:
    constexpr void clear_if_needed()
    {
        if (m_has_value)
            clear();
    }

    constexpr void clear()
    {
        value().~T();
        m_has_value = false;
    }

    constexpr T* storage()
    {
        return reinterpret_cast<T*>(m_storage);
    }
    constexpr T const* storage() const
    {
        return reinterpret_cast<T const*>(m_storage);
    }

    alignas(T) u8 m_storage[sizeof(T)];
    bool m_has_value { false };
};

template <typename T>
struct [[nodiscard]] Optional<T*> {

    constexpr Optional() = default;

    constexpr Optional(T* value)
        : m_value(value)
    {
    }

    ~Optional() { m_value = nullptr; }

    constexpr Optional(Optional&& other)
        : m_value(other.m_value)
    {
    }

    constexpr Optional& operator=(Optional&& other)
    {
        if (this == &other)
            return *this;
        m_value = other.m_value;
        return *this;
    }

    constexpr T*& value() { VERIFY(has_value()); return m_value; }
    constexpr T const* const& value() const { VERIFY(has_value()); return m_value; }

    constexpr T* release_value()
    {
        VERIFY(has_value());
        auto value = m_value;
        m_value = nullptr;
        return value;
    }

    constexpr bool has_value() const { return m_value != nullptr; }
    constexpr T* operator->() { VERIFY(has_value()); return value(); }

    constexpr T const* operator->() const { VERIFY(has_value()); return value(); }

    constexpr T* operator*() { VERIFY(has_value()); return release_value(); }
    constexpr T const* operator*() const { VERIFY(has_value()); return release_value(); }

    explicit constexpr operator bool() const { return has_value(); }

    template <typename F>
    constexpr decltype(auto) or_throw(F error_callback)
    {
        using Return = ErrorOr<T*, decltype(error_callback())>;
        if (has_value())
            return Return(release_value());
        return Return(error_callback());
    }

    template <typename F>
    constexpr decltype(auto) or_else(F callback)
    {
        if (!has_value())
            return callback();
        using Return = decltype(callback());
        return Return(release_value());
    }

private:
    T* m_value { nullptr };
};

}

using namespace Ty;

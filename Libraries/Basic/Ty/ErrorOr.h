#pragma once
#include "./Error.h"
#include "./Move.h"
#include "./Optional.h"
#include "./Traits.h"

namespace Ty {

template <typename T, typename E>
struct [[nodiscard]] ErrorOr {
    constexpr ErrorOr(ErrorOr&& other)
        : m_state(other.m_state)
    {
        switch (m_state) {
        case State::Error: m_error = other.release_error(); break;
        case State::Value: m_value = other.release_value(); break;
        case State::Moved: break;
        }
        other.m_state = State::Moved;
    }

    constexpr ErrorOr(T&& value) requires(!is_trivially_copyable<T>)
        : m_value(move(value))
        , m_state(State::Value)
    {
    }

    constexpr ErrorOr(T value) requires(is_trivially_copyable<T>)
        : m_value(value)
        , m_state(State::Value)
    {
    }

    constexpr ErrorOr(E&& error) requires(!is_trivially_copyable<E>)
        : m_error(move(error))
        , m_state(State::Error)
    {
    }

    constexpr ErrorOr(E error) requires(is_trivially_copyable<E>)
        : m_error(error)
        , m_state(State::Error)
    {
    }

    template <typename EE>
    constexpr ErrorOr(EE error) requires(
        !is_same<EE, Error> && is_constructible<E, EE>)
        : m_error(E(error))
        , m_state(State::Error)
    {
    }

    constexpr ErrorOr()
        : m_value(T())
        , m_state(State::Value)
    {
    }

    constexpr ~ErrorOr()
    {
        switch (m_state) {
        case State::Error: m_error.~E(); break;
        case State::Value: m_value.~T(); break;
        case State::Moved: break;
        }
    }

    constexpr ErrorOr& operator=(ErrorOr&& other)
    {
        m_state = other.m_state;
        if (m_state == State::Value)
            m_value = move(other.m_value);
        if (m_state == State::Error)
            m_error = move(other.m_error);
        other.m_state = State::Moved;
        return *this;
    }

    constexpr bool has_value() const
    {
        return m_state == State::Value;
    }
    constexpr bool is_error() const
    {
        return m_state == State::Error;
    }

    constexpr T release_value()
    {
        VERIFY(m_state == State::Value);
        m_state = State::Moved;
        return move(m_value);
    }

    constexpr E release_error()
    {
        VERIFY(m_state == State::Error);
        m_state = State::Moved;
        return move(m_error);
    }

    T const* operator->() const { return &value(); }

    constexpr T const& value() const
    {
        VERIFY(m_state == State::Value);
        return m_value;
    }

    constexpr E const& error() const
    {
        VERIFY(m_state == State::Error);
        return m_error;
    }

    template <typename F>
    constexpr ErrorOr<T, E> or_else(F callback)
    {
        if (is_error())
            return callback(release_error());
        return release_value();
    }

    constexpr T or_default(T default_value)
    {
        if (is_error()) {
            return default_value;
        }
        return release_value();
    }

    template <typename F>
    constexpr decltype(auto) then(F callback)
    {
        using Return = decltype(callback(release_value()));
        if (is_error())
            return Return(release_error());
        return Return(callback(release_value()));
    }

    constexpr bool operator==(T other) const
    {
        if (!has_value())
            return false;
        return value() == other;
    }

    constexpr void ignore() const { }

private:
    union {
        T m_value;
        E m_error;
    };
    enum class State : u8 {
        Value,
        Error,
        Moved,
    };
    State m_state;
};

template <typename E>
struct [[nodiscard]] ErrorOr<void, E> {
    constexpr ErrorOr() = default;

    constexpr ErrorOr(E error) requires is_trivially_copyable<E>
        : m_error(error)
    {
    }

    constexpr ErrorOr(E&& error) requires(!is_trivially_copyable<E>)
        : m_error(move(error))
    {
    }

    template <typename EE>
    constexpr ErrorOr(EE error) requires(
        !is_same<E, EE> && is_constructible<E, EE>)
        : m_error(EE(error))
    {
    }

    constexpr ErrorOr& operator=(ErrorOr&& other)
    {
        m_error = move(other.m_error);
    }

    constexpr bool has_value() const { return !is_error(); }
    constexpr bool is_error() const { return m_error.has_value(); }

    constexpr void release_value() const { }
    constexpr E release_error() { return m_error.release_value(); }

    constexpr E const& error() const { return m_error.value(); }

    template <typename F>
    decltype(auto) or_else(F callback)
    {
        using Return = decltype(callback(release_error()));
        if (is_error())
            return callback(release_error());
        return Return();
    }

    constexpr void ignore() const { }

private:
    Optional<E> m_error {};
};

}

using namespace Ty;

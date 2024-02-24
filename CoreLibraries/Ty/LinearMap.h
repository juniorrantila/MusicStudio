#pragma once
#include "ErrorOr.h"
#include "Move.h"
#include "Try.h"
#include "Vector.h"
#include "Verify.h"

namespace Ty {

template <typename Key, typename Value>
struct LinearMap {
    static ErrorOr<LinearMap> create()
    {
        return LinearMap {
            TRY(Vector<Key>::create()),
            TRY(Vector<Value>::create()),
        };
    }

    constexpr ErrorOr<void> append(Key key, Value value) requires(
        is_trivially_copyable<Key>and is_trivially_copyable<Value>)
    {
        TRY(m_keys.append(key));
        TRY(m_values.append(value));

        return {};
    }

    constexpr ErrorOr<void> append(Key&& key, Value value) requires(
        !is_trivially_copyable<
            Key> and is_trivially_copyable<Value>)
    {
        TRY(m_keys.append(move(key)));
        TRY(m_values.append(value));

        return {};
    }

    constexpr ErrorOr<void> append(Key key, Value&& value) requires(
        is_trivially_copyable<
            Key> and !is_trivially_copyable<Value>)
    {
        TRY(m_keys.append(key));
        TRY(m_values.append(move(value)));

        return {};
    }

    constexpr ErrorOr<void>
    append(Key&& key, Value&& value) requires(
        !is_trivially_copyable<
            Key> and !is_trivially_copyable<Value>)
    {
        TRY(m_keys.append(move(key)));
        TRY(m_values.append(move(value)));

        return {};
    }

    constexpr Optional<Id<Value>> find(Key const& key) const
    {
        for (u32 i = 0; i < m_keys.size(); i++) {
            if (m_keys[i] == key)
                return Id<Value>(i);
        }
        return {};
    }

    template <typename F>
    constexpr decltype(auto) find(Key const& key,
        F error_callback) const
    {
        using Return
            = ErrorOr<Id<Value>, decltype(error_callback())>;
        for (u32 i = 0; i < m_keys.size(); i++) {
            if (m_keys[i] == key)
                return Return(Id<Value>(i));
        }
        return Return(error_callback());
    }

    template <typename F>
    constexpr decltype(auto) fetch(Key const& key,
        F error_callback) const
    {
        using Return = ErrorOr<Value, decltype(error_callback())>;
        for (u32 i = 0; i < m_keys.size(); i++) {
            if (m_keys[i] == key)
                return Return(operator[](Id<Value>(i)));
        }
        return Return(error_callback());
    }

    constexpr ErrorOr<Value> fetch(Key const& key,
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        u32 line = __builtin_LINE()) const
    {
        for (u32 i = 0; i < m_keys.size(); i++) {
            if (m_keys[i] == key)
                return operator[](Id<Value>(i));
        }
        return Error::from_string_literal("value not in map",
            function, file, line);
    }

    constexpr Value const& operator[](Id<Value> id) const
    {
        VERIFY(id.raw() < m_values.size());
        return m_values[id];
    }

    constexpr Value& operator[](Id<Value> id)
    {
        VERIFY(id.raw() < m_values.size());
        return m_values[id];
    }

private:
    constexpr LinearMap(Vector<Key>&& keys, Vector<Value>&& values)
        : m_keys(move(keys))
        , m_values(move(values))
    {
    }

    Vector<Key> m_keys;
    Vector<Value> m_values;
};

}

using Ty::LinearMap; // NOLINT

#pragma once
#include "./ErrorOr.h"
#include "./Move.h"
#include "./Try.h"
#include "./Vector.h"
#include "./Verify.h"

namespace Ty {

template <typename Key, typename Value>
struct LinearMap {
    constexpr LinearMap() = default;

    static ErrorOr<LinearMap> create()
    {
        return LinearMap {
            TRY(Vector<Key>::create()),
            TRY(Vector<Value>::create()),
        };
    }

    constexpr ErrorOr<void> set(Key key, Value value) requires(
        is_trivially_copyable<Key>and is_trivially_copyable<Value>)
    {
        if (auto index = find(key)) {
            m_values[index->raw()] = value;
            return {};
        }
        TRY(m_keys.append(key));
        TRY(m_values.append(value));

        return {};
    }

    constexpr ErrorOr<Id<Value>> set(Key&& key, Value value) requires(
        !is_trivially_copyable<
            Key> and is_trivially_copyable<Value>)
    {
        if (auto index = find(key)) {
            m_values[index->raw()] = value;
            return *index;
        }

        auto raw_id = size();

        TRY(m_keys.append(move(key)));
        TRY(m_values.append(value));

        return Id<Value>(raw_id);
    }

    constexpr ErrorOr<Id<Value>> set(Key key, Value&& value) requires(
        is_trivially_copyable<
            Key> and !is_trivially_copyable<Value>)
    {
        if (auto index = find(key)) {
            m_values[index->raw()] = move(value);
            return *index;
        }

        auto raw_id = size();

        TRY(m_keys.append(key));
        TRY(m_values.append(move(value)));

        return Id<Value>(raw_id);
    }

    constexpr ErrorOr<Id<Value>>
    set(Key&& key, Value&& value) requires(
        !is_trivially_copyable<
            Key> and !is_trivially_copyable<Value>)
    {
        if (auto index = find(key)) {
            m_values[index->raw()] = move(value);
            return *index;
        }
        auto raw_id = size();

        TRY(m_keys.append(move(key)));
        TRY(m_values.append(move(value)));

        return Id<Value>(raw_id);
    }

    template <typename Other>
    constexpr Optional<Id<Value>> find(Other const& key) const
    {
        for (u32 i = 0; i < m_keys.size(); i++) {
            if (m_keys[i] == key)
                return Id<Value>(i);
        }
        return {};
    }

    template <typename Other, typename F>
    constexpr decltype(auto) find(Other const& key,
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

    template <typename Other, typename F>
    constexpr decltype(auto) fetch(Other const& key,
        F error_callback) const
    {
        using Return = ErrorOr<Value, decltype(error_callback())>;
        for (u32 i = 0; i < m_keys.size(); i++) {
            if (m_keys[i] == key)
                return Return(operator[](Id<Value>(i)));
        }
        return Return(error_callback());
    }

    template <typename Other>
    constexpr Optional<Value> fetch(Other const& key) const
    {
        for (u32 i = 0; i < m_keys.size(); i++) {
            if (m_keys[i] == key)
                return operator[](Id<Value>(i));
        }
        return {};
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

    View<Key const> keys() const { return m_keys; }
    View<Value const> values() const { return m_values; }
    usize size() const { return m_keys.size(); }

    constexpr bool has(Key const& key) const
    {
        return find(key).has_value();
    }

private:
    constexpr LinearMap(Vector<Key>&& keys, Vector<Value>&& values)
        : m_keys(move(keys))
        , m_values(move(values))
    {
    }

    Vector<Key> m_keys {};
    Vector<Value> m_values {};
};

}

using Ty::LinearMap; // NOLINT

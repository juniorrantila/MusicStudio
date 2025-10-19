#pragma once
#include "Move.h"
#include "SmallVector.h"
#include "Try.h"

namespace Ty {

template <typename Key, typename Value, u32 capacity = 16>
struct SmallMap {
    constexpr SmallMap() = default;

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

    constexpr Value const& operator[](Id<Value> id) const
    {
        return m_values[id];
    }

    constexpr Value& operator[](Id<Value> id)
    {
        return m_values[id];
    }

private:
    SmallVector<Key, capacity> m_keys {};
    SmallVector<Value, capacity> m_values {};
};

}

using Ty::SmallMap;

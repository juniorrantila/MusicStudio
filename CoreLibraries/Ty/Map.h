#pragma once
#include "Move.h"
#include "Try.h"
#include "Vector.h"

namespace Ty {

template <typename Key, typename Value>
struct Map {
    static ErrorOr<Map> create();

    constexpr ErrorOr<void> append(Key key, Value value)
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

    constexpr Value const& operator[](Id<Value> id) const { return m_values[id]; }
    constexpr Value& operator[](Id<Value> id) { return m_values[id]; }

private:
    constexpr Map(Vector<Key>&& keys, Vector<Key>&& values)
        : m_keys(move(keys))
        , m_values(move(values))
    {
    }

    Vector<Key> m_keys {};
    Vector<Value> m_values {};
};

}

using Ty::Map;

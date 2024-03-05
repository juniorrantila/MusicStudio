#pragma once
#include "Base.h"
#include "Bytes.h"
#include "Forward.h"
#include "Optional.h"
#include "Traits.h"

namespace Ty {

constexpr StringView operator""sv(c_string data, usize size);
struct StringView {
    constexpr StringView() = default;

    [[gnu::flatten]] static constexpr StringView from_c_string(
        c_string data)
    {
        return StringView(data, length_of(data));
    }

    static constexpr StringView from_parts(char const* data, u32 size)
    {
        return StringView(data, size);
    }

    constexpr StringView(char const* data, u32 size)
        : m_data(data)
        , m_size(size)
    {
    }

    consteval StringView(c_string data)
        : m_data(data)
        , m_size(data == nullptr ? 0 : __builtin_strlen(data))
    {
    }

    [[gnu::flatten]] constexpr bool operator==(
        StringView other) const
    {
        if (m_size != other.size())
            return false;

        if (!is_constant_evaluated()) {
            if (m_data == other.data())
                return true;
        }

        bool same = true;
        // clang-format off
        while (other.m_size --> 0)
            same &= m_data[other.size()] == other[other.size()];
        // clang-format on
        return same;
    }

    constexpr bool is_empty() const { return data() == nullptr || size() == 0; }

    constexpr char const& operator[](u32 index) const
    {
        return data()[index];
    }

    constexpr bool contains(char character) const
    {
        for (u32 i = 0; i < size(); i++) {
            if (data()[i] == character)
                return true;
        }
        return false;
    }

    [[gnu::flatten]] constexpr u32 unchecked_copy_to(char* other,
        u32 size) const
    {
        if (m_data == other)
            return m_size;
        for (u32 i = 0; i < size; i++)
            other[i] = m_data[i];
        return size;
    }

    [[gnu::flatten]] constexpr u32 unchecked_copy_to(
        char* __restrict other) const
    {
        return strncpy(other, *this);
    }

    [[gnu::always_inline]] constexpr StringView sub_view(u32 start, u32 size) const
    {
        auto remaining = m_size - start;
        if (remaining < size) [[unlikely]] {
            size = remaining;
        }
        return { &m_data[start], size };
    }

    constexpr StringView part(u32 start, u32 end) const
    {
        return { &m_data[start], end - start };
    }

    constexpr StringView shrink(u32 amount) const
    {
        return { data(), size() - amount };
    }

    constexpr bool starts_with(StringView other) const
    {
        if (m_size < other.size())
            return false;
        return sub_view(0, other.size()) == other;
    }

    constexpr bool ends_with(StringView other) const
    {
        if (m_size < other.size())
            return false;
        return part(m_size - other.size(), m_size) == other;
    }

    constexpr StringView shrink_from_start(u32 amount) const
    {
        return { &m_data[amount], m_size - amount };
    }
    constexpr StringView chop_left(u32 amount) const
    {
        return shrink_from_start(amount);
    }

    ErrorOr<Vector<StringView>> split_on(char character) const;
    ErrorOr<Vector<StringView>> split_on(StringView sequence) const;
    ErrorOr<Vector<u32>> find_all(char character) const;
    ErrorOr<Vector<u32>> find_all(StringView sequence) const;
    Optional<u32> find_first(char character) const
    {
        for (u32 i = 0; i < m_size; i++) {
            if (m_data[i] == character)
                return i;
        }
        return {};
    }

    constexpr StringView remove_trailing_null() const
    {
        auto view = *this;
        for (u32 i = view.size(); i -- > 0;) {
            if (view[i] == '\0')
                view.m_size--;
        }
        return view;
    }

    Bytes as_bytes() const { return Bytes(data(), size()); }

    explicit constexpr operator bool() const { return !is_empty(); }

    constexpr char const* begin() const { return data(); }
    constexpr char const* end() const { return data() + size(); }

    ErrorOr<StringBuffer> join(View<StringView const>) const;

    constexpr char const* data() const { return m_data; }
    constexpr u32 size() const { return m_size; }

    ErrorOr<StringBuffer> resolve_path(StringView root = ""sv) const;

private:
    [[gnu::flatten]] static constexpr u32 strncpy(
        char* __restrict to, StringView from)
    {
        if (from.data() == to)
            return from.size();
        for (u32 i = 0; i < from.size(); i++)
            to[i] = from[i];
        return from.size();
    }

    static constexpr u32 length_of(c_string string)
    {
        u32 size = 0;
        for (; string[size] != '\0'; ++size)
            ;
        return size;
    }

    char const* m_data { "" };
    u32 m_size { 0 };
};

constexpr StringView operator""sv(c_string data, usize size)
{
    return StringView(data, size);
}

}

using namespace Ty;

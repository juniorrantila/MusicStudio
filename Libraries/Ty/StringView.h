#pragma once
#include "Base.h"
#include "Bytes.h"
#include "Forward.h"
#include "Optional.h"
#include "Traits.h"

namespace Ty {

struct StringView {
    char const* data { "" };
    u32 size { 0 };

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
        : data(data)
        , size(size)
    {
    }

    consteval StringView(c_string data)
        : data(data)
        , size(__builtin_strlen(data))
    {
    }

    [[gnu::flatten]] constexpr bool operator==(
        StringView other) const
    {
        if (size != other.size)
            return false;

        if (!is_constant_evaluated()) {
            if (data == other.data)
                return true;
        }

        bool same = true;
        // clang-format off
        while (other.size --> 0)
            same &= data[other.size] == other.data[other.size];
        // clang-format on
        return same;
    }

    constexpr bool is_empty() const { return size == 0; }

    constexpr char const& operator[](u32 index) const
    {
        return data[index];
    }

    constexpr bool contains(char character) const
    {
        for (u32 i = 0; i < size; i++) {
            if (data[i] == character)
                return true;
        }
        return false;
    }

    [[gnu::flatten]] constexpr u32 unchecked_copy_to(char* other,
        u32 size) const
    {
        if (data == other)
            return size;
        for (u32 i = 0; i < size; i++)
            other[i] = data[i];
        return size;
    }

    [[gnu::flatten]] constexpr u32 unchecked_copy_to(
        char* __restrict other) const
    {
        return strncpy(other, *this);
    }

    [[gnu::always_inline]] constexpr StringView sub_view(u32 start,
        u32 size) const
    {
        auto remaining = this->size - start;
        if (remaining < size) [[unlikely]] {
            size = remaining;
        }
        return { &data[start], size };
    }

    constexpr StringView part(u32 start, u32 end) const
    {
        return { &data[start], end - start };
    }

    constexpr StringView shrink(u32 amount) const
    {
        return { data, size - amount };
    }

    constexpr bool starts_with(StringView other) const
    {
        if (size < other.size)
            return false;
        return sub_view(0, other.size) == other;
    }

    constexpr bool ends_with(StringView other) const
    {
        if (size < other.size)
            return false;
        return part(size - other.size, size) == other;
    }

    constexpr StringView shrink_from_start(u32 amount) const
    {
        return { &data[amount], size - amount };
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
        for (u32 i = 0; i < size; i++) {
            if (data[i] == character)
                return i;
        }
        return {};
    }

    constexpr StringView remove_trailing_null() const{
        auto view = *this;
        for (u32 i = view.size; i -- > 0;) {
            if (view[i] == '\0')
                view.size--;
        }
        return view;
    }

    Bytes as_bytes() const { return Bytes(data, size); }

    explicit constexpr operator bool() const { return !is_empty(); }

    char const* begin() const { return data; }
    char const* end() const { return data + size; }

    ErrorOr<StringBuffer> join(View<StringView const>) const;

private:
    [[gnu::flatten]] static constexpr u32 strncpy(
        char* __restrict to, StringView from)
    {
        if (from.data == to)
            return from.size;
        for (u32 i = 0; i < from.size; i++)
            to[i] = from[i];
        return from.size;
    }

    static constexpr u32 length_of(c_string string)
    {
        u32 size = 0;
        for (; string[size] != '\0'; ++size)
            ;
        return size;
    }
};

constexpr StringView operator""sv(c_string data, usize size)
{
    return StringView(data, size);
}

}

using namespace Ty;

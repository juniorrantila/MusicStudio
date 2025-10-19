#pragma once
#include "./Base.h"
#include "./Forward.h"
#include "./Optional.h"
#include "./Traits.h"

#include <Basic/StringSlice.h>
#include <Basic/Bytes.h>

namespace Ty {

constexpr StringView operator""sv(c_string data, usize size);
struct StringView {
    constexpr StringView() = default;

    constexpr StringView(StringSlice inner)
        : inner(inner)
    {
    }

    constexpr StringView(char const* data, u32 size)
        : inner({
            .items = data,
            .count = size,
        })
    {
    }

    consteval StringView(c_string data)
        : inner({
            .items = data,
            .count = data == nullptr ? 0 : __builtin_strlen(data),
        })
    {
    }

    StringView(View<char const> view);
    StringView(View<char> view);

    static constexpr StringView from_c_string(c_string data)
    {
        return sv_from_c_string(data);
    }

    static constexpr StringView from_c_string_with_max_size(c_string data, usize max_size)
    {
        return sv_from_c_string_with_max_size(data, max_size);
    }

    static constexpr StringView from_parts(char const* data, u32 size)
    {
        return sv_from_parts(data, size);
    }

    bool operator==(StringView other) const
    {
        return sv_equal(inner, other.inner);
    }

    bool is_empty() const { return sv_is_empty(inner); }

    char const& operator[](u32 index) const
    {
        VERIFY(index < size());
        return data()[index];
    }

    bool contains(char character) const { return sv_contains(inner, character); }

    u32 unchecked_copy_to(char* __restrict other) const { return strncpy(other, *this); }

    StringView sub_view(u32 start, u32 size) const { return sv_slice(inner, start, size); }

    StringView part(u32 start, u32 end) const { return sv_part(inner, start, end); }

    constexpr StringView shrink(u32 amount) const
    {
        return { data(), size() - amount };
    }

    bool starts_with(StringView other) const { return sv_starts_with(inner, other.inner); }
    bool ends_with(StringView other) const { return sv_ends_with(inner, other.inner); }

    StringView shrink_from_start(u32 amount) const { return sv_chop_left(inner, amount); }
    StringView chop_left(u32 amount) const { return sv_chop_left(inner, amount); }

    ErrorOr<Vector<StringView>> split_on(char character) const;
    ErrorOr<Vector<StringView>> split_on(StringView sequence) const;
    ErrorOr<Vector<u32>> find_all(char character) const;
    ErrorOr<Vector<u32>> find_all(StringView sequence) const;
    Optional<u32> find_first(char character) const
    {
        u64 index = 0;
        if (sv_find_first_char(inner, character, &index))
            return index;
        return {};
    }

    constexpr StringView remove_trailing_null() const
    {
        auto view = inner;
        for (u32 i = view.count; i -- > 0;) {
            if (view.items[i] == '\0')
                view.count--;
        }
        return view;
    }

    Bytes as_bytes() const { return bytes(data(), size()); }

    explicit constexpr operator bool() const { return !is_empty(); }

    constexpr char const* begin() const { return data(); }
    constexpr char const* end() const { return data() + size(); }

    ErrorOr<StringBuffer> join(View<StringView const>) const;

    constexpr char const* data() const { return inner.items; }
    constexpr u32 size() const { return inner.count; }

    ErrorOr<StringBuffer> resolve_path(StringView root = ""sv) const;

    constexpr operator StringSlice() const { return { data(), size() }; }

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

    static constexpr u32 length_of_or_max(c_string string, usize max)
    {
        u32 size = 0;
        for (; string[size] != '\0'; ++size) {
            if (size >= max) return max;
        }
        return size;
    }

    StringSlice inner;
};

constexpr StringView operator""sv(c_string data, usize size)
{
    return StringView(data, size);
}

}

using Ty::StringView;

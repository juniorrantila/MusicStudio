#include "./StringView.h"

#include "./Verify.h"
#include "./Defer.h"

#include <string.h>
#include <sys/syslimits.h>

StringView2 StringView2::from_parts(char const* s, usize count) { return sv_from_parts(s, count); }
C_API StringView2 sv_from_parts(char const* s, usize count)
{
    return (StringView2){
        .items = s,
        .count = count,
    };
}

StringView2 StringView2::from_c_string(c_string s) { return sv_from_c_string(s); }
C_API StringView2 sv_from_c_string(c_string s)
{
    if (!s) return sv_empty();
    return sv_from_parts(s, strlen(s));
}

StringView2 StringView2::from_c_string_with_max_size(c_string s, u64 max_size) { return sv_from_c_string_with_max_size(s, max_size); }
C_API StringView2 sv_from_c_string_with_max_size(c_string s, u64 max_size)
{
    if (!s) return sv_empty();
    return sv_from_parts(s, strnlen(s, max_size));
}

StringView2 StringView2::empty() { return sv_empty(); }
C_API StringView2 sv_empty(void)
{
    return sv_from_parts(0, 0);
}

[[nodiscard]] bool StringView2::join(Allocator* a, StringView2 const* parts, u64 count, StringView2* out) const { return sv_join(a, *this, parts, count, out); }
C_API [[nodiscard]] bool sv_join(Allocator* a, StringView2 delimiter, StringView2 const* parts, u64 count, StringView2* out)
{
    if (count == 0)
        return sv_clone(a, sv_empty(), out);
    if (count == 1)
        return sv_clone(a, parts[0], out);

    u64 len = delimiter.count * count - 1;
    for (u64 i = 0; i < count; i++)
        len += parts[i].count;
    char* storage = (char*)memalloc(a, len, 1);
    if (!storage) return false;

    char* dest = storage;
    for (u64 i = 0; i < count - 1; i++) {
        StringView2 part = parts[i];
        memcpy(dest, part.items, part.count);
        dest += part.count;
        memcpy(dest, delimiter.items, delimiter.count);
        dest += delimiter.count;
    }

    StringView2 last = parts[count - 1];
    memcpy(dest, last.items, last.count);

    *out = sv_from_parts(storage, len);
    return true;
}

[[nodiscard]] bool StringView2::clone(Allocator* a, StringView2* out) const { return sv_clone(a, *this, out); }
C_API [[nodiscard]] bool sv_clone(Allocator* a, StringView2 s, StringView2* out)
{
    char const* buf = (char const*)memclone(a, s.items, s.count, 1);
    if (!buf) return false;
    *out = sv_from_parts(buf, s.count);
    return true;
}

bool StringView2::is_empty() const { return sv_is_empty(*this); }
C_API bool sv_is_empty(StringView2 s)
{
    if (s.items == nullptr) return true;
    if (s.count == 0) return true;
    return false;
}

bool StringView2::equal(StringView2 other) const { return sv_equal(*this, other); }
C_API bool sv_equal(StringView2 a, StringView2 b)
{
    if (a.count != b.count) return false;
    if (a.items == b.items) return true;
    if (a.count == 0) return true;
    if (*a.items != *b.items) return false;
    return memcmp(a.items + 1, b.items + 1, a.count - 1) == 0;
}

StringView2 StringView2::slice(usize start, usize size) const { return sv_slice(*this, start, size); }
C_API StringView2 sv_slice(StringView2 s, usize start, usize size)
{
    if (sv_is_empty(s)) return sv_from_parts(s.items, 0);
    if (s.count - start < size) size = s.count - start;
    return sv_from_parts(s.items + start, size);
}

StringView2 StringView2::part(usize start, usize end) const { return sv_part(*this, start, end); }
C_API StringView2 sv_part(StringView2 s, usize start, usize end)
{
    VERIFY(end >= start);
    if (sv_is_empty(s)) return sv_from_parts(s.items, 0);
    return sv_slice(s, start, end - start);
}

StringView2 StringView2::chop_left(usize count) const { return sv_chop_left(*this, count); }
StringView2 sv_chop_left(StringView2 s, usize count)
{
    return sv_part(s, count, s.count);
}

bool StringView2::contains(char character) const { return sv_contains(*this, character); }
C_API bool sv_contains(StringView2 s, char character)
{
    for (u64 i = 0; i < s.count; i++) {
        if (s.items[i] == character)
            return true;
    }
    return false;
}

bool StringView2::starts_with(StringView2 s) const { return sv_starts_with(*this, s); }
C_API bool sv_starts_with(StringView2 s, StringView2 pattern)
{
    if (s.count < pattern.count)
        return false;
    return memcmp(s.items, pattern.items, pattern.count) == 0;
}

bool StringView2::ends_with(StringView2 s) const { return sv_ends_with(*this, s); }
C_API bool sv_ends_with(StringView2 s, StringView2 pattern)
{
    if (s.count < pattern.count)
        return false;
    return memcmp(s.items + s.count - pattern.count, pattern.items, pattern.count) == 0;
}

[[nodiscard]] bool StringView2::split_on(Allocator* a, char character, StringView2** items, u64* count) const { return sv_split_char(a, *this, character, items, count); }
C_API [[nodiscard]] bool sv_split_char(Allocator* a, StringView2 s, char character, StringView2** out_items, u64* out_count)
{
    u64* indexes = 0;
    u64 index_count = 0;
    if (!s.find_all(a, character, &indexes, &index_count))
        return false;
    defer [&] { memfree(a, indexes, index_count * sizeof(u64), alignof(u64)); };

    if (index_count == 0) {
        StringView2* items = (StringView2*)memclone(a, &s, sizeof(s), alignof(StringView2));
        if (!items) return false;
        *out_items = items;
        return true;
    }

    StringView2* items = (StringView2*)memalloc(a, index_count + 1 * sizeof(StringView2), alignof(StringView2));
    if (!items) return false;
    u64 count = 0;

    i64 last_index = -1;
    for (u64 i = 0; i < index_count; i++) {
        u64 index = indexes[i];
        items[count++] = sv_part(s, last_index + 1, index);
        last_index = (i64)index;
    }
    items[count++] = sv_part(s, last_index + 1, s.count);

    *out_items = items;
    *out_count = count;
    return true;
}

[[nodiscard]] bool StringView2::split_on(Allocator* a, StringView2 sequence, StringView2** items, u64* count) const { return sv_split_on(a, *this, sequence, items, count); }
C_API [[nodiscard]] bool sv_split_on(Allocator* a, StringView2 s, StringView2 sequence, StringView2** out_items, u64* out_count)
{
    u64* indexes = 0;
    u64 index_count = 0;
    if (!s.find_all(a, sequence, &indexes, &index_count))
        return false;
    defer [&] { memfree(a, indexes, sizeof(*indexes) * index_count, alignof(u64)); };

    if (index_count == 0) {
        StringView2* items = (StringView2*)memclone(a, &s, sizeof(s), alignof(StringView2));
        if (!items) return false;
        *out_items = items;
        return true;
    }

    StringView2* items = (StringView2*)memalloc(a, index_count + 1 * sizeof(StringView2), alignof(StringView2));
    if (!items) return false;
    u64 count = 0;

    i64 last_index = 0 - (i64)sequence.count;
    for (u64 i = 0; i < index_count; i++) {
        u64 index = indexes[i];
        items[count++] = sv_part(s, last_index + sequence.count, index);
        last_index = (i64)index;
    }
    items[count++] = sv_part(s, last_index + sequence.count, s.count);

    *out_items = items;
    *out_count = count;
    return true;
}

[[nodiscard]] bool StringView2::find_all(Allocator* a, char character, u64** items, u64* count) const { return sv_find_all_char(a, *this, character, items, count); }
C_API [[nodiscard]] bool sv_find_all_char(Allocator* a, StringView2 s, char character, u64** out_items, u64* out_count)
{
    u64 count = 0;
    for (u64 i = 0; i < s.count; i++) {
        if (s.items[i] == character)
            count += 1;
    }
    if (count == 0) {
        *out_count = 0;
        return true;
    }

    u64* items = (u64*)memalloc(a, count * sizeof(u64), alignof(u64));
    if (!items) return false;

    u64 out_i = 0;
    for (u64 i = 0; i < s.count; i++) {
        if (s.items[i] == character)
            items[out_i++] = i;
    }

    *out_items = items;
    *out_count = count;
    return true;
}

[[nodiscard]] bool StringView2::find_all(Allocator* a, StringView2 sequence, u64** items, u64* count) const { return sv_find_all(a, *this, sequence, items, count); }
C_API [[nodiscard]] bool sv_find_all(Allocator* a, StringView2 s, StringView2 sequence, u64** out_items, u64* out_count)
{
    u64 count = 0;
    for (u64 i = 0; i < s.count;) {
        if (sv_starts_with(sv_part(s, i, s.count), sequence)) {
            count += 1;
            i += sequence.count;
        } else {
            i += 1;
        }
    }
    if (count == 0) {
        *out_count = 0;
        return true;
    }

    u64* items = (u64*)memalloc(a, count * sizeof(u64), alignof(u64));
    if (!items) return false;

    u64 out_i = 0;
    for (u64 i = 0; i < s.count; i++) {
        if (sv_starts_with(sv_part(s, i, s.count), sequence)) {
            items[out_i++] = i;
            i += sequence.count;
        } else {
            i += 1;
        }
    }

    *out_items = items;
    *out_count = count;
    return true;
}

[[nodiscard]] bool StringView2::find_first(char character, u64* out) const { return sv_find_first_char(*this, character, out); }
C_API [[nodiscard]] bool sv_find_first_char(StringView2 s, char character, u64* out)
{
    for (u64 i = 0; i < s.count; i++) {
        if (s.items[i] == character) {
            *out = i;
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool StringView2::resolve_path(Allocator* a, StringView2 root, StringView2* out) const { return sv_resolve_path(a, *this, root, out); }
C_API [[nodiscard]] bool sv_resolve_path(Allocator* a, StringView2 s, StringView2 root, StringView2* out)
{
    StringView2* parts = 0;
    u64 part_count = 0;
    if (!s.split_on(a, '/', &parts, &part_count)) return false;
    defer [&] { memfree(a, parts, sizeof(StringView2) * part_count, alignof(StringView2)); };

    StringView2 segments[1024];
    segments[0] = root;
    u64 depth = 1;

    for (u64 i = 0; i < part_count; i++) {
        auto part = parts[i];
        if (part.equal("."_sv))
            continue;
        if (part.equal(".."_sv)) {
            depth -= 1;
            if (depth == 0) return false; // Trying to escape root
            continue;
        }
        if (depth + 1 >= ty_array_size(segments))
            return false;
        segments[depth++] = part;
    }

    return sv_join(a, "/"_sv, segments, depth, out);
}

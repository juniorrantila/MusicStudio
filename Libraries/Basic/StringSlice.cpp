#include "./StringSlice.h"

#include "./Verify.h"
#include "./Defer.h"
#include "./Bytes.h"

#include "./Context.h"
#include "./StringBuilder.h"
#include "./String.h"

#include <string.h>
#include <sys/syslimits.h>

C_API StringSlice sv_from_parts(char const* s, u64 count)
{
    return (StringSlice){
        .items = s,
        .count = count,
    };
}

C_API StringSlice sv_from_c_string(c_string s)
{
    if (!s) return sv_empty();
    return sv_from_parts(s, strlen(s));
}

C_API StringSlice sv_from_c_string_with_max_size(c_string s, u64 max_size)
{
    if (!s) return sv_empty();
    return sv_from_parts(s, strnlen(s, max_size));
}

C_API StringSlice sv_empty(void)
{
    return sv_from_parts(0, 0);
}

C_API Bytes sv_bytes(StringSlice s)
{
    return (Bytes){
        .items = (u8 const*)s.items,
        .count = s.count,
    };
}

C_API [[nodiscard]] bool sv_join(Allocator* a, StringSlice delimiter, StringSlice const* parts, u64 count, StringSlice* out)
{
    if (count == 0)
        return sv_clone(a, sv_empty(), out);

    StringBuilder sb = string_builder_init();

    for (u64 i = 0; i < count; i++) {
        if (auto err = sb.append(parts[i]); !err.ok)
            return false;
        if (i + 1 != count) {
            if (auto err = sb.append(delimiter); !err.ok)
                return false;
        }
    }

    return sb.build_view(out);
}

C_API [[nodiscard]] bool sv_clone(Allocator* a, StringSlice s, StringSlice* out)
{
    char const* buf = (char const*)memclone(a, s.items, s.count, 1);
    if (!buf) return false;
    *out = sv_from_parts(buf, s.count);
    return true;
}

C_API bool sv_is_empty(StringSlice s)
{
    if (s.items == nullptr) return true;
    if (s.count == 0) return true;
    return false;
}

C_API bool sv_equal(StringSlice a, StringSlice b)
{
    if (a.count != b.count) return false;
    if (a.items == b.items) return true;
    if (a.count == 0) return true;
    if (*a.items != *b.items) return false;
    return memcmp(a.items + 1, b.items + 1, a.count - 1) == 0;
}

C_API StringSlice sv_slice(StringSlice s, u64 start, u64 size)
{
    if (sv_is_empty(s)) return sv_from_parts(s.items, 0);
    if (s.count - start < size) size = s.count - start;
    return sv_from_parts(s.items + start, size);
}

C_API StringSlice sv_part(StringSlice s, u64 start, u64 end)
{
    VERIFY(end >= start);
    if (sv_is_empty(s)) return sv_from_parts(s.items, 0);
    return sv_slice(s, start, end - start);
}

StringSlice sv_chop_left(StringSlice s, u64 count)
{
    return sv_part(s, count, s.count);
}

C_API bool sv_contains(StringSlice s, char character)
{
    for (u64 i = 0; i < s.count; i++) {
        if (s.items[i] == character)
            return true;
    }
    return false;
}

C_API bool sv_starts_with(StringSlice s, StringSlice pattern)
{
    if (s.count < pattern.count)
        return false;
    return memcmp(s.items, pattern.items, pattern.count) == 0;
}

C_API bool sv_ends_with(StringSlice s, StringSlice pattern)
{
    if (s.count < pattern.count)
        return false;
    return memcmp(s.items + s.count - pattern.count, pattern.items, pattern.count) == 0;
}

C_API [[nodiscard]] bool sv_split_char(Allocator* a, StringSlice s, char character, StringSlice** out_items, u64* out_count)
{
    u64* indexes = 0;
    u64 index_count = 0;
    if (!sv_find_all_char(temporary_arena(), s, character, &indexes, &index_count))
        return false;

    if (index_count == 0) {
        StringSlice* items = (StringSlice*)memclone(a, &s, sizeof(s), alignof(StringSlice));
        if (!items) return false;
        *out_items = items;
        return true;
    }

    StringSlice* items = (StringSlice*)memalloc(a, (index_count + 1) * sizeof(StringSlice), alignof(StringSlice));
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

C_API [[nodiscard]] bool sv_split_on(Allocator* a, StringSlice s, StringSlice sequence, StringSlice** out_items, u64* out_count)
{
    u64* indexes = 0;
    u64 index_count = 0;
    if (!sv_find_all(a, s, sequence, &indexes, &index_count))
        return false;
    defer [&] { memfree(a, indexes, sizeof(*indexes) * index_count, alignof(u64)); };

    if (index_count == 0) {
        StringSlice* items = (StringSlice*)memclone(a, &s, sizeof(s), alignof(StringSlice));
        if (!items) return false;
        *out_items = items;
        return true;
    }

    StringSlice* items = (StringSlice*)memalloc(a, index_count + 1 * sizeof(StringSlice), alignof(StringSlice));
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

C_API [[nodiscard]] bool sv_find_all_char(Allocator* a, StringSlice s, char character, u64** out_items, u64* out_count)
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

C_API [[nodiscard]] bool sv_find_all(Allocator* a, StringSlice s, StringSlice sequence, u64** out_items, u64* out_count)
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

C_API [[nodiscard]] bool sv_find_first_char(StringSlice s, char character, u64* out)
{
    for (u64 i = 0; i < s.count; i++) {
        if (s.items[i] == character) {
            *out = i;
            return true;
        }
    }
    return false;
}

C_API [[nodiscard]] bool sv_resolve_path(Allocator* a, StringSlice s, StringSlice root, StringSlice* out)
{
    StringSlice* parts = 0;
    u64 part_count = 0;
    if (!sv_split_char(temporary_arena(), s, '/', &parts, &part_count)) return false;

    StringSlice segments[1024];
    segments[0] = root;
    u64 depth = 1;

    for (u64 i = 0; i < part_count; i++) {
        auto part = parts[i];
        if (sv_equal(part, "."_sv))
            continue;
        if (sv_equal(part, ".."_sv)) {
            depth -= 1;
            if (depth == 0) return false; // Trying to escape root
            continue;
        }
        if (depth + 1 >= ARRAY_SIZE(segments))
            return false;
        segments[depth++] = part;
    }
    if (depth == 1) {
        return sv_clone(a, root, out);
    }

    // FIXME: Feels like we shouldn't slice off the root.
    return sv_join(a, "/"_sv, &segments[1], depth - 1, out);
}

C_API c_string sv_fmt(StringSlice s)
{
    char* c = (char*)tpush(s.count + 1, 1);
    if (!c) return nullptr;
    ty_memcpy(c, s.items, s.count);
    c[s.count] = 0;
    return c;
}

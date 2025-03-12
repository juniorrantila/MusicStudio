#include "./RopeString.h"

#include "./StringView.h"
#include "./Verify.h"

#include <string.h>

static const usize iteration_limit = 1'000'000'000; // Surely no string would be larger than 239 GB

RopeString* RopeString::from_parts(Allocator* a, char const* buf, usize buf_size) { return rs_from_parts(a, buf, buf_size); }
C_API RopeString* rs_from_parts(Allocator* a, char const* buf, usize buf_size)
{
    return rs_from_view(a, sv_from_parts(buf, buf_size));
}

RopeString* RopeString::from_view(Allocator* a, StringView string) { return rs_from_view(a, string); }
C_API RopeString* rs_from_view(Allocator* a, StringView string)
{
    RopeString* begin = (RopeString*)memalloc(a, sizeof(RopeString), alignof(RopeString));
    if (!begin) return nullptr;
    *begin = {
        .items = {},
        .size_left = sizeof(begin->items),
        .next = nullptr,
        .gpa = a,
    };
    if (string.is_empty()) return begin;
    if (string.count <= begin->size_left) {
        memcpy(begin->items, string.items, string.count);
        begin->size_left = sizeof(begin->items) - string.count;
        return begin;
    }
    RopeString* previous = begin;
    while (!string.is_empty()) {
        RopeString* s = (RopeString*)memalloc(a, sizeof(RopeString), alignof(RopeString));
        if (!s) goto fi_1;

        previous->next = s;
        string = sv_part(string, sizeof(s->items), string.count);
    }
    return begin;
fi_1:
    rs_destroy(begin);
    return nullptr;
}

void RopeString::destroy() { rs_destroy(this); }
C_API void rs_destroy(RopeString* s)
{
    for (usize limit = 0; s && limit < iteration_limit; limit++) {
        RopeString* next = s->next;
        memfree(s->gpa, s, sizeof(RopeString), alignof(RopeString));
        memset(s, 0xAB, sizeof(*s));
        s = next;
    }
}

usize RopeString::segment_size() const { return rs_segment_size(this); }
C_API usize rs_segment_size(RopeString const* s)
{
    return sizeof(s->items) - s->size_left;
}

usize RopeString::size() const { return rs_size(this); }
C_API usize rs_size(RopeString const* s)
{
    usize size = 0;
    for (usize limit = 0; s && limit < iteration_limit; s = s->next, limit++) {
        size += rs_segment_size(s);
    }
    return size;
}

usize RopeString::allocation_size() const { return rs_allocation_size(this); }
C_API usize rs_allocation_size(RopeString const* s)
{
    return rs_segment_count(s) * sizeof(RopeString);
}

usize RopeString::segment_count() const { return rs_segment_count(this); }
C_API usize rs_segment_count(RopeString const* s)
{
    usize count = 0;
    for (usize limit = 0; limit < iteration_limit && s; s = s->next, limit++) {
        count += 1;
    }
    return count;
}

bool RopeString::is_empty() const { return rs_is_empty(this); }
C_API bool rs_is_empty(RopeString const* s)
{
    return s->size() == 0;
}

bool RopeString::equal(RopeString const* other) const { return rs_equal(this, other); }
C_API bool rs_equal(RopeString const* a, RopeString const* b)
{
    for (usize limit = 0; limit < iteration_limit; limit++) {
        if (!a->next && b->next) return false;
        if (!b->next && a->next) return false;
        StringView as = sv_from_parts(a->items, rs_segment_size(a));
        StringView bs = sv_from_parts(a->items, rs_segment_size(b));
        if (!sv_equal(as, bs)) return false;
        if (!a->next) break;
        a = a->next;
        b = b->next;
    }
    return true;
}

bool RopeString::equal(StringView other) const { return rs_equal_sv(this, other); }
C_API bool rs_equal_sv(RopeString const* a, StringView b)
{
    if (a->size() != b.count) return false;
    for (usize limit = 0; !b.is_empty() && limit < iteration_limit; limit++) {
        StringView as = sv_from_parts(a->items, a->segment_size());
        StringView bs = b.slice(0, a->segment_size());
        if (!sv_equal(as, bs)) return false;
        a = a->next;
        b = b.chop_left(a->segment_size());
    }
    return true;
}

usize RopeString::copy_into(char* buf, usize buf_size) const { return rs_copy_into(this, buf, buf_size); }
C_API usize rs_copy_into(RopeString const* s, char* buf, usize buf_size)
{
    usize buf_index = 0;
    for (usize limit = 0; s && limit < iteration_limit; s = s->next, limit++) {
        usize segment_size = rs_segment_size(s);
        for (usize i = 0; i < segment_size; buf_index++, i++) {
            if (buf_index >= buf_size)
                return buf_index;
            buf[buf_index] = s->items[i];
        }
    }
    return buf_index;
}

RopeString* RopeString::copy(Allocator* a) const { return rs_copy(this, a); }
C_API RopeString* rs_copy(RopeString const* s, Allocator* a)
{
    if (!s) return nullptr;
    RopeString* begin = (RopeString*)memclone(a, s, sizeof(*s), alignof(RopeString));
    if (!begin) return nullptr;
    begin->gpa = a;
    RopeString* previous = begin;
    for (; s; s = s->next) {
        RopeString* other = (RopeString*)memclone(a, s, sizeof(*s), alignof(RopeString));
        if (!other) goto fi_1;
        other->gpa = a;
        previous->next = other;
        previous = other;
    }
    return begin;
fi_1:
    rs_destroy(begin);
    return nullptr;
}

void RopeString::append(RopeString* node) { return rs_append(this, node); }
C_API void rs_append(RopeString* s, RopeString* node)
{
    while (s) {
        VERIFYS(s != node, "rope string may not be cyclic");
        s = s->next;
    }
    VERIFY(s->next == nullptr);
    s->next = node;
}

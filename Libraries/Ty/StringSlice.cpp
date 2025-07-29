#include "./StringSlice.h"

#include "./StringView.h"
#include "./StringBuffer.h"

#include <string.h>

Optional<StringSlice> StringSlice::resolve_path(Allocator* a) const
{
    StringSlice result;
    if (!string_resolve_path(*this, a, &result)) {
        return {};
    }
    return result;
}
C_API bool string_resolve_path(StringSlice s, Allocator* a, StringSlice* out)
{
    // FIXME: Use temporary allocator when splitting path
    auto result = StringView::from_parts(s.items, s.count).resolve_path();
    if (result.is_error()) return false;
    auto view = result->view();

    char* buf = (char*)memclone(a, view.data(), view.size(), 1);
    if (!buf) return false;

    *out = string_slice(buf, view.size());
    return true;
}


bool StringSlice::equal(StringSlice other) const { return string_slice_equal(other, *this); }
bool string_slice_equal(StringSlice a, StringSlice b)
{
    if (a.count != b.count) return false;
    if (a.items == b.items) return true;
    if (a.count == 0) return true;
    return a.items[0] == b.items[0] && memcmp(a.items + 1, b.items + 1, a.count - 1) == 0;
}

C_API bool string_clone(Allocator* a, StringSlice s, StringSlice* out)
{
    VERIFY(s.count);
    char* ptr = (char*)memclone(a, s.items, s.count, 1);
    if (!ptr) return false;
    *out = string_slice(ptr, s.count);
    return true;
}

StringView StringSlice::as_view() const
{
    return StringView::from_parts(items, count);
}

Bytes StringSlice::as_bytes() const
{
    return bytes(items, count);
}

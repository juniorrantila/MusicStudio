#include "./StringView.h"

#include "./Verify.h"

#include <string.h>

StringView StringView::from_parts(char const* s, usize count) { return sv_from_parts(s, count); }
C_API StringView sv_from_parts(char const* s, usize count)
{
    return (StringView){
        .items = s,
        .count = count,
    };
}

StringView StringView::from_c_string(c_string s) { return sv_from_c_string(s); }
C_API StringView sv_from_c_string(c_string s)
{
    if (!s) return sv_empty();
    return sv_from_parts(s, strlen(s));
}

StringView StringView::empty() { return sv_empty(); }
C_API StringView sv_empty(void)
{
    return sv_from_parts(0, 0);
}

StringView StringView::clone(Allocator* a) const { return sv_clone(a, *this); }
C_API StringView sv_clone(Allocator* a, StringView s)
{
    if (sv_is_empty(s)) return sv_empty();
    char const* buf = (char const*)memclone(a, s.items, s.count, 1);
    if (!buf) return sv_empty();
    return sv_from_parts(buf, s.count);
}

bool StringView::is_empty() const { return sv_is_empty(*this); }
C_API bool sv_is_empty(StringView s)
{
    if (s.items == nullptr) return true;
    if (s.count == 0) return true;
    return false;
}

bool StringView::equal(StringView other) const { return sv_equal(*this, other); }
C_API bool sv_equal(StringView a, StringView b)
{
    if (a.count != b.count) return false;
    if (a.items == b.items) return true;
    if (a.count == 0) return true;
    if (*a.items != *b.items) return false;
    return memcmp(a.items + 1, b.items + 1, a.count - 1) == 0;
}

StringView StringView::slice(usize start, usize size) const { return sv_slice(*this, start, size); }
C_API StringView sv_slice(StringView s, usize start, usize size)
{
    if (sv_is_empty(s)) return sv_from_parts(s.items, 0);
    if (s.count - start < size) size = s.count - start;
    return sv_from_parts(s.items + start, size);
}

StringView StringView::part(usize start, usize end) const { return sv_part(*this, start, end); }
C_API StringView sv_part(StringView s, usize start, usize end)
{
    VERIFY(end >= start);
    if (sv_is_empty(s)) return sv_from_parts(s.items, 0);
    return sv_slice(s, start, end - start);
}

StringView StringView::chop_left(usize count) const { return sv_chop_left(*this, count); }
StringView sv_chop_left(StringView s, usize count)
{
    return sv_part(s, count, s.count);
}

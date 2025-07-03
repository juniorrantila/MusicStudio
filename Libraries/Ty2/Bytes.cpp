#include "./Bytes.h"

#include "./StringView.h"

StringView2 Bytes::as_view() const { return bytes_as_view(*this); }
C_API StringView2 bytes_as_view(Bytes bytes)
{
    return sv_from_parts((char const*)bytes.items, bytes.count);
}

bool Bytes::equal(Bytes other) const { return bytes_equal(*this, other); }
C_API bool bytes_equal(Bytes a, Bytes b)
{
    if (a.count != b.count)
        return false;
    if (a.items == b.items)
        return true;
    return __builtin_memcmp(a.items, b.items, a.count) == 0;
}

Bytes Bytes::slice(u64 begin, u64 count) const { return bytes_slice(*this, begin, count); }
C_API Bytes bytes_slice(Bytes b, u64 begin, u64 count)
{
    if (begin >= b.count)
        return bytes(b.items, 0);
    if (begin + count > b.count)
        count = b.count - begin;
    return bytes(b.items + begin, count);
}

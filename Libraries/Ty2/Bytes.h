#pragma once
#include "./Base.h"

#include "./StringView.h"

typedef struct Bytes {
    u8 const* items;
    u64 count;

#ifdef __cplusplus
    StringView2 as_view() const;

    bool equal(Bytes) const;

    Bytes slice(u64 begin, u64 count) const;
#endif
} Bytes;

C_INLINE Bytes bytes(void const* items, u64 count)
{
    return (Bytes){
        .items = (u8 const*)items,
        .count = count
    };
}

C_API StringView2 bytes_as_view(Bytes bytes);
C_API bool bytes_equal(Bytes, Bytes);
C_API Bytes bytes_slice(Bytes, u64 begin, u64 count);

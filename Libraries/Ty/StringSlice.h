#pragma once
#include "./Base.h"

#ifdef __cplusplus
#include "./Forward.h"
#endif

#include <Ty2/Allocator.h>

typedef struct StringSlice {
    char const* items;
    usize count;

#ifdef __cplusplus
    bool equal(StringSlice) const;

    Optional<StringSlice> resolve_path(Allocator*) const;

    StringView as_view() const;
#endif
} StringSlice;

C_API inline StringSlice string_slice_empty(void) { return (StringSlice){ 0, 0 }; }
C_API inline StringSlice string_slice_from_c_string(c_string s) { return (StringSlice){ s, __builtin_strlen(s) }; }
C_API inline StringSlice string_slice(char const* items, usize count) { return (StringSlice){ items, count }; }
C_API bool string_clone(Allocator*, StringSlice s, StringSlice*);

C_API bool string_resolve_path(StringSlice, Allocator*, StringSlice*);
C_API bool string_slice_equal(StringSlice, StringSlice);

#ifdef __cplusplus
constexpr StringSlice operator""s (char const* data, unsigned long size)
{
    return {
        .items = data,
        .count = size,
    };
}
#endif

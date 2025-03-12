#pragma once
#include "./Base.h"

#include "./Allocator.h"

typedef struct StringView {
    char const* items;
    usize count;

#ifdef __cplusplus
    static StringView from_parts(char const*, usize);
    static StringView from_c_string(c_string);
    static StringView empty();

    StringView clone(Allocator*) const;

    bool is_empty() const;

    bool equal(StringView other) const;
    StringView slice(usize start, usize size) const;
    StringView part(usize start, usize end) const;

    StringView chop_left(usize count) const;
#endif
} StringView;

C_API StringView sv_from_parts(char const*, usize);
C_API StringView sv_from_c_string(c_string);
C_API StringView sv_clone(Allocator*, StringView);

C_API bool sv_is_empty(StringView s);
C_API StringView sv_empty(void);

C_API bool sv_equal(StringView, StringView);

C_API StringView sv_slice(StringView, usize start, usize size);
C_API StringView sv_part(StringView, usize start, usize end);

C_API StringView sv_chop_left(StringView, usize count);

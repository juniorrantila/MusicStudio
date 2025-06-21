#pragma once
#include "./Base.h"

#include "./Allocator.h"

typedef struct StringView2 {
    char const* items;
    usize count;

#ifdef __cplusplus
    static StringView2 from_parts(char const*, usize);
    static StringView2 from_c_string(c_string);
    static StringView2 empty();

    StringView2 clone(Allocator*) const;

    bool is_empty() const;

    bool equal(StringView2 other) const;
    StringView2 slice(usize start, usize size) const;
    StringView2 part(usize start, usize end) const;

    StringView2 chop_left(usize count) const;
#endif
} StringView2;

C_API StringView2 sv_from_parts(char const*, usize);
C_API StringView2 sv_from_c_string(c_string);
C_API StringView2 sv_clone(Allocator*, StringView2);

C_API bool sv_is_empty(StringView2 s);
C_API StringView2 sv_empty(void);

C_API bool sv_equal(StringView2, StringView2);

C_API StringView2 sv_slice(StringView2, usize start, usize size);
C_API StringView2 sv_part(StringView2, usize start, usize end);

C_API StringView2 sv_chop_left(StringView2, usize count);

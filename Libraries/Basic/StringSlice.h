#pragma once
#include "./Forward.h"
#include "./Types.h"

#include "./Allocator.h"

typedef struct StringSlice {
    char const* items;
    uptr count;
} StringSlice;
#define SV(chars) (StringSlice){("" chars ""), sizeof("" chars "") - 1}

C_API StringSlice sv_from_parts(char const*, uptr);
C_API StringSlice sv_from_c_string(c_string);
C_API StringSlice sv_from_c_string_with_max_size(c_string, u64);

C_API Bytes sv_bytes(StringSlice);

C_API [[nodiscard]] bool sv_clone(Allocator*, StringSlice, StringSlice*);
C_API [[nodiscard]] bool sv_join(Allocator*, StringSlice delimiter, StringSlice const* parts, u64 count, StringSlice*);

C_API bool sv_is_empty(StringSlice s);
C_API StringSlice sv_empty(void);

C_API bool sv_equal(StringSlice, StringSlice);

C_API StringSlice sv_slice(StringSlice, uptr start, uptr size);
C_API StringSlice sv_part(StringSlice, uptr start, uptr end);

C_API StringSlice sv_chop_left(StringSlice, uptr count);

C_API bool sv_contains(StringSlice, char character);
C_API bool sv_starts_with(StringSlice, StringSlice);
C_API bool sv_ends_with(StringSlice, StringSlice);

C_API c_string sv_fmt(StringSlice);

C_API [[nodiscard]] bool sv_split_char(Allocator*, StringSlice, char character, StringSlice**, u64*);
C_API [[nodiscard]] bool sv_split_on(Allocator*, StringSlice, StringSlice sequence, StringSlice**, u64*);

C_API [[nodiscard]] bool sv_find_all_char(Allocator*, StringSlice, char character, u64**, u64*);
C_API [[nodiscard]] bool sv_find_all(Allocator*, StringSlice, StringSlice sequence, u64**, u64*);
C_API [[nodiscard]] bool sv_find_first_char(StringSlice, char character, u64*);

C_API [[nodiscard]] bool sv_resolve_path(Allocator*, StringSlice, StringSlice root, StringSlice*);

#ifdef __cplusplus
constexpr StringSlice operator""_sv (char const* data, unsigned long size)
{
    return (StringSlice){
        .items = data,
        .count = size,
    };
}

constexpr StringSlice operator""s (char const* data, unsigned long size)
{
    return {
        .items = data,
        .count = size,
    };
}
#endif

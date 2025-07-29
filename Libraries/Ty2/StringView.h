#pragma once
#include "./Base.h"

#include "./Allocator.h"

typedef struct StringView2 {
    char const* items;
    usize count;

#ifdef __cplusplus
    static StringView2 from_parts(char const*, usize);
    static StringView2 from_c_string(c_string);
    static StringView2 from_c_string_with_max_size(c_string, u64);
    static StringView2 empty();

    [[nodiscard]] bool join(Allocator* a, StringView2 const* parts, u64 count, StringView2* out) const;

    [[nodiscard]] bool clone(Allocator*, StringView2*) const;

    bool is_empty() const;

    bool equal(StringView2 other) const;
    StringView2 slice(usize start, usize size) const;
    StringView2 part(usize start, usize end) const;

    StringView2 chop_left(usize count) const;

    bool contains(char character) const;
    bool starts_with(StringView2) const;
    bool ends_with(StringView2) const;

    [[nodiscard]] bool split_on(Allocator*, char character, StringView2**, u64*) const;
    [[nodiscard]] bool split_on(Allocator*, StringView2, StringView2**, u64*) const;

    [[nodiscard]] bool find_all(Allocator*, char character, u64**, u64*) const;
    [[nodiscard]] bool find_all(Allocator*, StringView2, u64**, u64*) const;
    [[nodiscard]] bool find_first(char character, u64*) const;

    [[nodiscard]] bool resolve_path(Allocator*, StringView2 root, StringView2*) const;
#endif
} StringView2;

C_API StringView2 sv_from_parts(char const*, usize);
C_API StringView2 sv_from_c_string(c_string);
C_API StringView2 sv_from_c_string_with_max_size(c_string, u64);
C_API [[nodiscard]] bool sv_clone(Allocator*, StringView2, StringView2*);
C_API [[nodiscard]] bool sv_join(Allocator*, StringView2 delimiter, StringView2 const* parts, u64 count, StringView2*);

C_API bool sv_is_empty(StringView2 s);
C_API StringView2 sv_empty(void);

C_API bool sv_equal(StringView2, StringView2);

C_API StringView2 sv_slice(StringView2, usize start, usize size);
C_API StringView2 sv_part(StringView2, usize start, usize end);

C_API StringView2 sv_chop_left(StringView2, usize count);

C_API bool sv_contains(StringView2, char character);
C_API bool sv_starts_with(StringView2, StringView2);
C_API bool sv_ends_with(StringView2, StringView2);

C_API [[nodiscard]] bool sv_split_char(Allocator*, StringView2, char character, StringView2**, u64*);
C_API [[nodiscard]] bool sv_split_on(Allocator*, StringView2, StringView2 sequence, StringView2**, u64*);

C_API [[nodiscard]] bool sv_find_all_char(Allocator*, StringView2, char character, u64**, u64*);
C_API [[nodiscard]] bool sv_find_all(Allocator*, StringView2, StringView2 sequence, u64**, u64*);
C_API [[nodiscard]] bool sv_find_first_char(StringView2, char character, u64*);

C_API [[nodiscard]] bool sv_resolve_path(Allocator*, StringView2, StringView2 root, StringView2*);

#ifdef __cplusplus
constexpr StringView2 operator""_sv (char const* data, unsigned long size)
{
    return (StringView2){
        .items = data,
        .count = size,
    };
}
#endif

#pragma once
#include <Ty/Base.h>

typedef struct {
    char *items;
    usize count;
    usize capacity;
} String_Builder;

#define SB_Fmt "%.*s"
#define SB_Arg(sb) (int) (sb).count, (sb).items

#define sb_append_buf da_append_many
#define sb_append_cstr(sb, cstr)  \
    do {                          \
        const char *s = (cstr);   \
        usize n = strlen(s);      \
        da_append_many(sb, s, n); \
    } while (0)
#define sb_append_null(sb) da_append_many(sb, "", 1)

#define sb_to_sv(sb) sv_from_parts((sb).items, (sb).count)


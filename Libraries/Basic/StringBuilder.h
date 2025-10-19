#pragma once
#include "./Base.h"

#include "./Error.h"
#include "./StringSlice.h"

typedef struct StringChunk StringChunk;
typedef struct StringChunk {
    StringChunk* next;
    StringSlice string;
} StringChunk;

typedef struct StringBuilder {
    StringChunk* head;
    StringChunk* tail;

#ifdef __cplusplus
    __attribute__((format(printf, 2, 0)))
    KError vappendf(c_string fmt, va_list);
    __attribute__((format(printf, 2, 3)))
    KError appendf(c_string fmt, ...);
    __attribute__((format(printf, 2, 3)))
    void mappendf(c_string fmt, ...);

    KError append(StringSlice);
    void mappend(StringSlice);

    KError indent(u64 amount);
    void mindent(u64 amount);

    c_string build(Allocator* = nullptr);
    [[nodiscard]] bool build_view(StringSlice*, Allocator* = nullptr);
#endif
} StringBuilder;

C_INLINE StringBuilder string_builder_init()
{
    return (StringBuilder){};
}

__attribute__((format(printf, 2, 0)))
C_API KError sb_vappendf(StringBuilder*, c_string fmt, va_list);
__attribute__((format(printf, 2, 3)))
C_API KError sb_appendf(StringBuilder*, c_string fmt, ...);
__attribute__((format(printf, 2, 3)))
C_API void sb_mappendf(StringBuilder*, c_string fmt, ...);

C_API KError sb_append(StringBuilder*, StringSlice);
C_API void sb_mappend(StringBuilder*, StringSlice);

C_API KError sb_indent(StringBuilder*, u64 amount);
C_API void sb_mindent(StringBuilder*, u64 amount);

C_API c_string sb_build(StringBuilder*, Allocator* IF_CPP(= nullptr));
C_API [[nodiscard]] bool sb_build_view(StringBuilder*, StringSlice*, Allocator* IF_CPP(= nullptr));

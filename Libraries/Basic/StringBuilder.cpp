#include "./StringBuilder.h"

#include "./Verify.h"
#include "./String.h"
#include "./StringSlice.h"
#include "./Context.h"

#include <errno.h>

KError StringBuilder::vappendf(c_string fmt, va_list args) { return sb_vappendf(this, fmt, args); }
C_API KError sb_vappendf(StringBuilder* sb, c_string fmt, va_list args)
{
    StringSlice s = sv_empty();
    if (auto err = tvprints(&s, fmt, args); !err.ok)
        return err;
    return sb_append(sb, s);
}

__attribute__((format(printf, 2, 3)))
KError StringBuilder::appendf(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    auto res = vappendf(fmt, args);
    va_end(args);
    return res;
}
__attribute__((format(printf, 2, 3)))
C_API KError sb_appendf(StringBuilder* sb, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    auto res = sb_vappendf(sb, fmt, args);
    va_end(args);
    return res;
}

__attribute__((format(printf, 2, 3)))
void StringBuilder::mappendf(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    auto res = vappendf(fmt, args);
    va_end(args);
    VERIFY(res.ok);
}
__attribute__((format(printf, 2, 3)))
C_API void sb_mappendf(StringBuilder* sb, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    auto res = sb_vappendf(sb, fmt, args);
    va_end(args);
    VERIFY(res.ok);
}

KError StringBuilder::append(StringSlice s) { return sb_append(this, s); }
KError sb_append(StringBuilder* sb, StringSlice s)
{
    StringChunk* chunk = tclone((StringChunk){
        .next = nullptr,
        .string = s,
    });
    if (!chunk) {
        return kerror_unix(ENOMEM);
    }
    if (!sb->head) {
        sb->head = chunk;
        sb->tail = chunk;
        return kerror_none;
    }
    sb->tail->next = chunk;
    sb->tail = chunk;
    return kerror_none;
}


void StringBuilder::mappend(StringSlice s) { return sb_mappend(this, s); }
void sb_mappend(StringBuilder* sb, StringSlice s)
{
    VERIFY(sb_append(sb, s).ok);
}

KError StringBuilder::indent(u64 amount) { return sb_indent(this, amount); }
C_API KError sb_indent(StringBuilder* sb, u64 amount)
{
    for (u64 i = 0; i < amount; i++) {
        auto e = sb->appendf("  ");
        if (!e.ok) return e;
    }
    return kerror_none;
}

void StringBuilder::mindent(u64 amount) { return sb_mindent(this, amount); }
C_API void sb_mindent(StringBuilder* sb, u64 amount)
{
    VERIFY(sb_indent(sb, amount).ok);
}

c_string StringBuilder::build(Allocator* a) { return sb_build(this, a); }
c_string sb_build(StringBuilder* sb, Allocator* a)
{
    if (a == nullptr) a = temporary_arena();
    u64 size = 0;
    for (StringChunk* chunk = sb->head; chunk; chunk = chunk->next)
        size += chunk->string.count;

    char* s = (char*)memalloc(a, size + 1, 1);
    if (!s) return nullptr;
    s[size] = '\0';

    u64 cursor = 0;
    for (StringChunk* chunk = sb->head; chunk; chunk = chunk->next) {
        ty_memcpy(&s[cursor], chunk->string.items, chunk->string.count);
        cursor += chunk->string.count;
    }
    sb->head = nullptr;
    sb->tail = nullptr;

    return s;
}

[[nodiscard]] bool StringBuilder::build_view(StringSlice* out, Allocator* a) { return sb_build_view(this, out, a); }
C_API [[nodiscard]] bool sb_build_view(StringBuilder* sb, StringSlice* out, Allocator* a)
{
    c_string s = sb_build(sb, a);
    if (!s) return false;
    *out = sv_from_c_string(s);
    return true;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wunused"
#pragma clang attribute push (__attribute__((no_sanitize("integer"))), apply_to=function)

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_NOUNALIGNED
#define STB_SPRINTF_STATIC
#define STB_SPRINTF_DECORATE(x) stb_##x
#include "./stb_sprintf.h"

#pragma clang attribute pop
#pragma clang diagnostic pop

#include "./FixedArena.h"

#include "./Allocator.h"
#include "./Base.h"
#include "./Verify.h"

static void* dispatch(Allocator*, AllocatorEvent);
static bool owns(FixedArena const* arena, void* ptr);

C_API FixedArena fixed_arena_init(void* memory, u64 size)
{
    VERIFY(memory != nullptr);
    VERIFY(size != 0);
    mempoison(memory, size);
    return {
        .allocator = make_allocator(dispatch),
        .base = (u8*)memory,
        .head = (u8*)memory,
        .end = ((u8*)memory) + size,
    };
}

u64 FixedArena::bytes_used() const { return fixed_arena_bytes_used(this); }
C_API u64 fixed_arena_bytes_used(FixedArena const* arena)
{
    return arena->head - arena->base;
}

void FixedArena::drain() { return fixed_arena_drain(this); }
C_API void fixed_arena_drain(FixedArena* arena)
{
    VERIFY(arena->head >= arena->base);
    VERIFY(arena->end >= arena->base);
    u64 size = ((u64)arena->end) - ((u64)arena->base);
    arena->head = arena->base;
    mempoison(arena->head, size);
}

FixedMark FixedArena::mark() const { return fixed_arena_mark(this); }
C_API FixedMark fixed_arena_mark(FixedArena const* arena)
{
    return make_fixed_mark(arena->head);
}

void FixedArena::sweep(FixedMark mark) { return fixed_arena_sweep(this, mark); }
C_API void fixed_arena_sweep(FixedArena* arena, FixedMark mark)
{
    VERIFY(owns(arena, mark.value));
    VERIFY(arena->head >= mark.value);
    VERIFY(mark.value != nullptr);
    uptr size = arena->head - mark.value;
    arena->head = mark.value;
    mempoison(arena->head, size);
}

void* FixedArena::push(u64 size, u64 align) { return fixed_arena_push(this, size, align); }
C_API void* fixed_arena_push(FixedArena* arena, u64 size, u64 align)
{
    u8* new_head = __builtin_align_up(arena->head, align);
    if ((new_head + size) > arena->end)
        return nullptr;
    arena->head = new_head;
    VERIFY(((uptr)arena->head) % align == 0);
    void* ptr = arena->head;
    arena->head += size;
    memunpoison(ptr, size);
    return ptr;
}

[[gnu::format(printf, 2, 3)]]
c_string FixedArena::fmt(c_string format, ...)
{
    va_list args;
    va_start(args, format);
    c_string result = vfmt(format, args);
    va_end(args);
    return result;
}
c_string fixed_arena_fmt(FixedArena* arena, c_string format, ...)
{
    va_list args;
    va_start(args, format);
    c_string result = fixed_arena_vfmt(arena, format, args);
    va_end(args);
    return result;
}

[[gnu::format(printf, 2, 0)]]
c_string FixedArena::vfmt(c_string format, va_list args) { return fixed_arena_vfmt(this, format, args); }
c_string fixed_arena_vfmt(FixedArena* arena, c_string format, va_list args)
{
    int len = stb_vsnprintf(nullptr, 0, format, args);
    if (len < 0) return nullptr;
    if (len == 0) return "";
    char* buf = (char*)arena->push(len + 1, 1);
    memzero(buf, len + 1);
    if (!buf) return nullptr;
    int len2 = stb_vsnprintf(buf, len + 1, format, args);
    VERIFY(len == len2);
    return buf;
}

[[gnu::format(printf, 2, 3)]]
c_string FixedArena::must_fmt(c_string format, ...)
{
    va_list args;
    va_start(args, format);
    c_string result = fixed_arena_must_vfmt(this, format, args);
    va_end(args);
    return result;
}
C_API [[gnu::format(printf, 2, 3)]]
c_string fixed_arena_must_fmt(FixedArena* arena, c_string format, ...)
{
    va_list args;
    va_start(args, format);
    c_string result = fixed_arena_must_vfmt(arena, format, args);
    va_end(args);
    return result;
}

[[gnu::format(printf, 2, 0)]]
c_string FixedArena::must_vfmt(c_string format, va_list args) { return fixed_arena_must_vfmt(this, format, args);  }
C_API [[gnu::format(printf, 2, 0)]]
c_string fixed_arena_must_vfmt(FixedArena* arena, c_string format, va_list args)
{
    c_string result = fixed_arena_vfmt(arena, format, args);
    VERIFY(result != nullptr);
    return result;
}

static bool owns(FixedArena const* arena, void* ptr)
{
    u8* m = (u8*)ptr;
    return m <= arena->end && m >= arena->base;
}

static void fixed_arena_free(FixedArena* arena, void* ptr, u64 size, u64 align)
{
    (void)align;
    VERIFYS(ptr != nullptr, "trying to free null pointer");
    VERIFY(owns(arena, ptr));
    VERIFY(arena->base <= (arena->head - size));
    mempoison(ptr, size);
    if (arena->head - size == ptr)
        arena->head -= size;
}


static void* dispatch(Allocator* a, AllocatorEvent event)
{
    FixedArena* arena = field_base(FixedArena, allocator, a);
    switch (event.tag) {
    case AllocatorEventTag_Alloc:
        return arena->push(event.byte_count, event.align);
    case AllocatorEventTag_Free:
        fixed_arena_free(arena, event.ptr, event.byte_count, event.align);
        return nullptr;
    case AllocatorEventTag_Owns:
        return owns(arena, event.ptr) ? (void*)1 : 0;
    }
    return nullptr;
}

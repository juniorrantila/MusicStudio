#include "./Allocator.h"

#include "./Verify.h"

#include <string.h>
#include <stb/sprintf.h>

void* Allocator::alloc(u64 byte_count, u64 align) { return memalloc(this, byte_count, align); }
void Allocator::free(void* ptr, u64 byte_count, u64 align) { return memfree(this, ptr, byte_count, align); }

bool Allocator::owns(void* ptr) { return memowns(this, ptr); }

void* Allocator::clone(void const* data, u64 byte_count, u64 align) { return memclone(this, data, byte_count, align); }
C_API void* memclone(Allocator* a, void const* data, u64 byte_count, u64 align)
{
    VERIFY(data != nullptr);
    void* n = memalloc(a, byte_count, align);
    if (!n) return nullptr;
    memcpy(n, data, byte_count);
    return n;
}

C_API void* memclone_zero_extend(Allocator* a, void const* data, u64 byte_count, u64 align, u64 extend_bytes)
{
    VERIFY(data != nullptr);
    void* n = memalloc(a, byte_count + extend_bytes, align);
    if (!n) return nullptr;
    memset(n, 0, byte_count + extend_bytes);
    memcpy(n, data, byte_count);
    return n;
}

// FIXME: Add optimization for arena allocators.
C_API void* memrealloc(Allocator* a, void const* data, u64 old_byte_count, u64 new_byte_count, u64 align)
{
    if (old_byte_count == new_byte_count) return (void*)data;
    if (!data) {
        VERIFY(old_byte_count == 0);
        return memalloc(a, new_byte_count, align);
    }
    if (new_byte_count == 0) {
        memfree(a, (void*)data, old_byte_count, align);
        return nullptr;
    }

    void* n = memalloc(a, new_byte_count, align);
    if (!n) return nullptr;
    u64 min = new_byte_count < old_byte_count ? new_byte_count : old_byte_count;
    memcpy(n, data, min);
    memfree(a, (void*)data, old_byte_count, align);
    return n;
}

c_string Allocator::vfmt(c_string fmt, va_list args) { return memvfmt(this, fmt, args); }
C_API c_string memvfmt(Allocator* a, c_string fmt, va_list args)
{
    int len = stb_vsnprintf(nullptr, 0, fmt, args);
    if (len < 0) return nullptr;
    if (len == 0) return "";
    char* buf = (char*)memalloc(a, len + 1, 1);
    memzero(buf, len + 1);
    if (!buf) return nullptr;
    int len2 = stb_vsnprintf(buf, len + 1, fmt, args);
    VERIFY(len == len2);
    return buf;
}

c_string Allocator::fmt(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    c_string result = memvfmt(this, fmt, args);
    va_end(args);
    return result;
}

C_API c_string memfmt(Allocator* a, c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    c_string result = memvfmt(a, fmt, args);
    va_end(args);
    return result;
}

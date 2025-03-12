#include "./Allocator.h"

#include "./Verify.h"

#include <string.h>

void* Allocator::alloc(usize byte_count, usize align) { return memalloc(this, byte_count, align); }
void Allocator::free(void* ptr, usize byte_count, usize align) { return memfree(this, ptr, byte_count, align); }

bool Allocator::owns(void* ptr) { return memowns(this, ptr); }

void* Allocator::clone(void const* data, usize byte_count, usize align) { return memclone(this, data, byte_count, align); }
C_API void* memclone(Allocator* a, void const* data, usize byte_count, usize align)
{
    VERIFY(data != nullptr);
    void* n = memalloc(a, byte_count, align);
    if (!n) return nullptr;
    memcpy(n, data, byte_count);
    return n;
}

C_API void* memclone_zero_extend(Allocator* a, void const* data, usize byte_count, usize align, usize extend_bytes)
{
    VERIFY(data != nullptr);
    void* n = memalloc(a, byte_count + extend_bytes, align);
    if (!n) return nullptr;
    memset(n, 0, byte_count + extend_bytes);
    memcpy(n, data, byte_count);
    return n;
}

C_API void* memrealloc(Allocator* a, void const* data, usize old_byte_count, usize new_byte_count, usize align)
{
    if (old_byte_count == new_byte_count) return (void*)data;
    if (!data) {
        VERIFY(old_byte_count == 0);
        return memalloc(a, new_byte_count, align);
    }
    // FIXME: What if alloc after this fails?
    memfree(a, (void*)data, old_byte_count, align);
    if (new_byte_count == 0) {
        return nullptr;
    }
    void* n = memalloc(a, new_byte_count, align);
    if (n == data) return n;
    memmove(n, data, old_byte_count);
    return n;
}

#include "./ArenaAllocator.h"

#include "./System.h"

namespace Ty {

ErrorOr<ArenaAllocator> ArenaAllocator::create(usize max_memory)
{
    u8* base = (u8*)TRY(System::mmap(max_memory, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON));
    return ArenaAllocator(Own, {base, max_memory});
}

void ArenaAllocator::destroy()
{
    usize size = m_end - m_base;
    MUST(System::munmap(m_base, size));
}


void* ArenaAllocator::ialloc(Allocator* allocator, usize size, usize align)
{
    auto* arena = ty_field_base(ArenaAllocator, m_allocator, allocator);
    auto align_diff = ((uptr)arena->m_head) % align;
    if ((arena->m_head + align_diff + size) > arena->m_end) {
        return nullptr;
    }
    arena->m_head += align_diff;
    auto* ptr = arena->m_head;
    arena->m_head += size;
    return ptr;
}

void ArenaAllocator::ifree(Allocator* allocator, void* data, usize size)
{
    auto* arena = ty_field_base(ArenaAllocator, m_allocator, allocator);
    if (arena->m_head - size == data) {
        arena->m_head -= size;
    }
}

}

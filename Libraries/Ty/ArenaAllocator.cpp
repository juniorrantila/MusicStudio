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
    usize size = m_arena.end - m_arena.base;
    MUST(System::munmap(m_arena.base, size));
    m_arena = {};
}

}

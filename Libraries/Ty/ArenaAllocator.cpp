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


ErrorOr<void*> ArenaAllocator::alloc(usize size, usize align, c_string function, c_string file, usize line)
{
    auto align_diff = ((uptr)m_head) % align;
    if ((m_head + align_diff + size) > m_end) {
        return Error::from_errno(ENOMEM, function, file, line);
    }
    m_head += align_diff;
    auto* ptr = m_head;
    m_head += size;
    return ptr;
}

}

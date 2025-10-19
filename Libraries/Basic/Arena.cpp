#include "./Arena.h"

#include "./Types.h"
#include "./Verify.h"
#include "./FixedArena.h"

#include <stb/sprintf.h>

FixedArena ArenaBase::fixed_arena()
{
    VERIFY(m_size != 0);
    VERIFY(m_size > m_head);
    return fixed_arena_init(m_buffer + m_head, m_size - m_head);
}

void ArenaBase::init(u64 size)
{
    VERIFY(size != 0);
    mempoison(m_buffer, size);
    m_head = 0;
    m_size = size;
}

u64 ArenaBase::bytes_used() const
{
    return m_head;
}

void ArenaBase::drain()
{
    m_head = 0;
    mempoison(m_buffer, m_size);
}

ArenaMark ArenaBase::mark() const
{
    return (ArenaMark){m_head};
}

void ArenaBase::sweep(ArenaMark mark)
{
    VERIFY(mark.value >= 0);
    VERIFY(mark.value < m_size);
    uptr size = m_head - mark.value;
    m_head = mark.value;
    mempoison(&m_buffer[m_head], size);
}

void* ArenaBase::push(u64 size, u64 align)
{
    u64 new_head = __builtin_align_up(m_head, align);
    if ((new_head + size) > m_size)
        return nullptr;
    m_head = new_head;
    VERIFY(m_head % align == 0);
    void* ptr = &m_buffer[m_head];
    m_head += size;
    memunpoison(ptr, size);
    return ptr;
}

c_string ArenaBase::fmt(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    c_string result = vfmt(fmt, args);
    va_end(args);
    return result;
}

c_string ArenaBase::vfmt(c_string fmt, va_list args)
{
    int len = stb_vsnprintf(nullptr, 0, fmt, args);
    if (len < 0) return nullptr;
    if (len == 0) return "";
    char* buf = (char*)push(len + 1, 1);
    memzero(buf, len + 1);
    if (!buf) return nullptr;
    int len2 = stb_vsnprintf(buf, len + 1, fmt, args);
    VERIFY(len == len2);
    return buf;
}

c_string ArenaBase::must_fmt(c_string fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    c_string result = vfmt(fmt, args);
    va_end(args);
    VERIFY(result != nullptr);
    return result;
}

c_string ArenaBase::must_vfmt(c_string fmt, va_list args)
{
    c_string result = vfmt(fmt, args);
    VERIFY(result != nullptr);
    return result;
}

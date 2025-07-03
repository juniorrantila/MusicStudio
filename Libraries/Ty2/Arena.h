#pragma once
#include "./Base.h"

#ifndef __cplusplus
#error "This file only works in C++"
#endif

struct FixedArena;
struct ArenaMark { u64 value; };
struct ArenaBase {
    u64 m_head;
    u64 m_size;
    u8 m_buffer[0];

    FixedArena fixed_arena();

    void* push(u64 size, u64 align);
    void drain();

    u64 bytes_used() const;

    ArenaMark mark() const;
    void sweep(ArenaMark mark);

    u8 const* base() const { return m_buffer; }
    u8 const* end() const { return m_buffer + m_size; }

protected:
    void init(u64 size);
};

template <u64 Size>
struct Arena : ArenaBase {
    u8 m_buffer[Size];

    void* push(u64 size, u64 align)
    {
        initialize_if_needed();
        return ArenaBase::push(size, align);
    }

    void drain()
    {
        initialize_if_needed();
        return ArenaBase::drain();
    }

    u64 bytes_used() const
    {
        initialize_if_needed();
        return ArenaBase::bytes_used();
    }

    ArenaMark mark() const
    {
        initialize_if_needed();
        return ArenaBase::mark();
    }

    void sweep(ArenaMark mark)
    {
        initialize_if_needed();
        return ArenaBase::sweep(mark);
    }

    u8 const* base() const
    {
        initialize_if_needed();
        return ArenaBase::base();
    }

    u8 const* end() const
    {
        initialize_if_needed();
        return ArenaBase::end();
    }

private:
    void initialize_if_needed() { if (m_size != Size) ArenaBase::init(Size); }
};
static_assert(ty_offsetof(Arena<1>, m_buffer) == ty_offsetof(ArenaBase, m_buffer));

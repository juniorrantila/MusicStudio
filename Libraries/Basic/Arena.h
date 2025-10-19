#pragma once
#include "./Types.h"
VALIDATE_IS_CPP();

#include "./String.h"

#include <stdarg.h>

struct FixedArena;
struct ArenaMark { u64 value; };
struct ArenaBase {
    u64 m_head = 0;
    u64 m_size = 0;
    u8 m_buffer[0];

    ArenaBase(u64 size) { init(size); }

    FixedArena fixed_arena();

    void* push(u64 size, u64 align);
    void drain();

    u64 bytes_used() const;

    ArenaMark mark() const;
    void sweep(ArenaMark mark);

    u8 const* base() const { return m_buffer; }
    u8 const* end() const { return m_buffer + m_size; }

    [[gnu::format(printf, 2, 3)]]
    c_string fmt(c_string, ...);

    [[gnu::format(printf, 2, 0)]]
    c_string vfmt(c_string, va_list);

    [[gnu::format(printf, 2, 3)]]
    c_string must_fmt(c_string, ...);

    [[gnu::format(printf, 2, 0)]]
    c_string must_vfmt(c_string, va_list);

    template <typename T>
    T* clone(T value) { return (T*)clone(&value, sizeof(value), alignof(T)); }

    void* clone(void* value, u64 size, u64 align)
    {
        void* new_value = push(size, align);
        if (!new_value)
            return nullptr;
        return ty_memcpy(new_value, value, size);
    }

protected:
    void init(u64 size);
};

template <u64 Size>
struct Arena : ArenaBase {
    u8 m_buffer[Size];

    constexpr Arena()
        : ArenaBase(Size)
    {
    }

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
static_assert(OFFSET_OF(Arena<1>, m_buffer) == OFFSET_OF(ArenaBase, m_buffer));

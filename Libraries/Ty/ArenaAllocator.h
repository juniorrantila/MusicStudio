#pragma once
#include "./Base.h"
#include "./ErrorOr.h"
#include "./View.h"

#include <Ty2/FixedArena.h>

namespace Ty {

struct ArenaAllocator {
    enum Kind {
        Borrow,
        Own,
    };

    constexpr ArenaAllocator() = default;

    static ErrorOr<ArenaAllocator> create(usize max_memory);

    constexpr ArenaAllocator(View<u8> view)
        : ArenaAllocator(Borrow, view)
    {
    }

    ArenaAllocator(ArenaAllocator const&) = delete;
    ArenaAllocator& operator=(ArenaAllocator const&) = delete;

    constexpr ArenaAllocator(ArenaAllocator&& other)
        : m_arena(other.m_arena)
        , m_kind(other.m_kind)
    {
        other.m_kind = Borrow;
        other.m_arena = {};
    }

    constexpr ArenaAllocator& operator=(ArenaAllocator&& other)
    {
        if (this == &other) {
            return *this;
        }
        this->~ArenaAllocator();
        new (this)ArenaAllocator(move(other));
        return *this;
    }

    ~ArenaAllocator()
    {
        drain();
        if (m_kind == Own) {
            destroy();
        }
    }

    Allocator* allocator() { return &m_arena.allocator; }
    operator Allocator*() { return allocator(); }
    Allocator* operator->() { return allocator(); }

    void drain() { m_arena.drain(); }
    usize size_used() const { return m_arena.bytes_used(); }

private:
    constexpr ArenaAllocator(Kind kind, View<u8> view)
        : m_arena(fixed_arena_from_slice(view.data(), view.size()))
        , m_kind(kind)
    {
    }

    void destroy();

    FixedArena m_arena;
    Kind m_kind;
};

}

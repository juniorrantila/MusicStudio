#pragma once
#include "./Base.h"
#include "./ErrorOr.h"
#include "./View.h"
#include "./Allocator.h"

namespace Ty {

struct ArenaAllocator : public Allocator {
    enum Kind {
        Borrow,
        Own,
    };

    constexpr ArenaAllocator() = default;

    static ErrorOr<ArenaAllocator> create(usize max_memory);
    constexpr bool is_initialized() const { return m_base != nullptr; }

    constexpr ArenaAllocator(View<u8> view)
        : ArenaAllocator(Borrow, view)
    {
    }

    ArenaAllocator(ArenaAllocator const&) = delete;
    ArenaAllocator& operator=(ArenaAllocator const&) = delete;

    constexpr ArenaAllocator(ArenaAllocator&& other)
        : m_base(other.m_base)
        , m_head(other.m_head)
        , m_end(other.m_end)
        , m_kind(other.m_kind)
    {
        other.m_kind = Borrow;
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

    constexpr ~ArenaAllocator()
    {
        drain();
        if (m_kind == Own) {
            destroy();
        }
    }

    ErrorOr<void*> raw_alloc(usize size, usize align,
        c_string func = __builtin_FUNCTION(), 
        c_string file = __builtin_FILE(),
        usize line = __builtin_LINE()) override;

    void raw_free(void* data, usize size) override;

    constexpr void drain() { m_head = m_base; }

    View<u8> pool() const
    {
        return View(m_base, m_end - m_base);
    }

    usize size_used() const
    {
        return m_head - m_base;
    }

private:
    constexpr ArenaAllocator(Kind kind, View<u8> view)
        : m_base(view.data())
        , m_head(view.data())
        , m_end(view.data() + view.size())
        , m_kind(kind)
    {
    }

    void destroy();

    u8* m_base { nullptr };
    u8* m_head { nullptr };
    u8* m_end { nullptr };
    Kind m_kind { Borrow };
};

}

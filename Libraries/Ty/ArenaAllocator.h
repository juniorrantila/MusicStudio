#pragma once
#include "./Base.h"
#include "./ErrorOr.h"
#include "./Try.h"
#include "./View.h"

namespace Ty {

struct ArenaAllocator {
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
        return *new (this)ArenaAllocator(move(other));
    }

    constexpr ~ArenaAllocator()
    {
        drain();
        if (m_kind == Own) {
            destroy();
        }
    }

    ErrorOr<void*> alloc(usize size, usize align,
        c_string func = __builtin_FUNCTION(), 
        c_string file = __builtin_FILE(),
        usize line = __builtin_LINE());

    constexpr void drain() { m_head = m_base; }

    template <typename T>
    ErrorOr<T*> alloc(
        c_string function = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        usize line = __builtin_LINE()
    ) {
        return (T*)TRY(alloc(sizeof(T), alignof(T), function, file, line));
    }

    template <typename T>
    ErrorOr<View<T>> alloc(
        usize size,
        c_string func = __builtin_FUNCTION(),
        c_string file = __builtin_FILE(),
        usize line = __builtin_LINE()
    ) {
        auto* data = (T*)TRY(alloc(size * sizeof(T), alignof(T), func, file, line));
        return View(data, size);
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

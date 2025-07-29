#pragma once
#if __cplusplus

#include "./Base.h"

namespace Ty2 {

namespace Internal {

static consteval u16 djb2(char const* bytes, usize size)
{
    u16 hash = 5381;
    for (usize i = 0; i < size; ++i) {
        hash = hash * 33 + (u8)bytes[i];
    }
    return hash;
}

template <typename T>
struct TypeTrick {
    static consteval u16 eval()
    {
        return djb2(__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__));
    }
};

template <typename T>
static constexpr u16 id = TypeTrick<T>::eval();

}

struct TypeId {
    explicit constexpr TypeId(u16 raw)
        : m_raw(raw)
    {
    }

    constexpr bool operator==(TypeId other) const { return m_raw == other.m_raw; }

    constexpr u16 raw() const { return m_raw; }

    constexpr operator u16() const
    {
        return m_raw;
    }

private:
    u16 m_raw { 0 };
};

template <typename T>
static consteval inline TypeId type_id()
{
    constexpr u16 id = Internal::id<T>;
    static_assert(id != 0, "0 type id is reserved for special values");
    static_assert(id != 0xFFFF, "0xFFFF type id is reserved for special values");
    return TypeId(id);
}

bool type_register(u16, c_string name);

}

#endif

#if __cplusplus
#define ty_type_register(T) [[maybe_unused]] static inline bool __ty2_register__##T = Ty2::type_register(Ty2::type_id<T>(), #T)
#else
#define ty_type_register(T)
#endif

C_API c_string ty_type_name(u16);

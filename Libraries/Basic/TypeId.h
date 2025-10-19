#pragma once
#if __cplusplus

#include "./Base.h"
#include "./Hash.h"

namespace Ty2 {

namespace Internal {

static consteval u16 djb2(char const* bytes, u64 size)
{
    u16 hash = 5381;
    for (u64 i = 0; i < size; ++i) {
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
static consteval TypeId type_id()
{
    constexpr u16 id = Internal::id<T>;
    static_assert(id != 0, "0 type id is reserved for special values");
    static_assert(id != 0xFFFF, "0xFFFF type id is reserved for special values");
    return TypeId(id);
}

}

#endif

C_API bool ty_type_register_impl(u16, c_string name);

#if __cplusplus
#define TYPE_REGISTER(T) [[maybe_unused]] static inline bool __ty2_register__##T = ty_type_register_impl(Ty2::type_id<T>(), #T)
#else
#define TYPE_ID(T) ({ [[maybe_unused]] T a; djb2_string(djb2_initial_seed, #T); })
#define TYPE_REGISTER_2(tag, T) ty_type_register_impl(tag, #T)
#define TYPE_REGISTER(T) [[gnu::constructor]] void __ty2_register__##T(void) { ty_type_register_impl(TYPE_ID(T), #T); } static_assert(1)
#endif

C_API c_string ty_type_name(u16);

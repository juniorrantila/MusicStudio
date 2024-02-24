#pragma once

namespace Ty {

using i8 = signed char;
using i16 = signed short;
using i32 = signed int;
using i64 = signed long;

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long;

using u128 = unsigned __int128;
using i128 = signed __int128;

static_assert(sizeof(i8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(i64) == 8);

static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);

using usize = u64;
using isize = i64;

template <u8 bytes>
struct TypeFromSize;

template <>
struct TypeFromSize<1> {
    using Signed = i8;
    using Unsigned = u8;
};

template <>
struct TypeFromSize<2> {
    using Signed = i16;
    using Unsigned = u16;
};

template <>
struct TypeFromSize<4> {
    using Signed = i32;
    using Unsigned = u32;
};

template <>
struct TypeFromSize<8> {
    using Signed = i64;
    using Unsigned = u64;
};

using iptr = typename TypeFromSize<sizeof(void*)>::Signed;
using uptr = typename TypeFromSize<sizeof(void*)>::Unsigned;

using f32 = float;
using f64 = double;

using c_string = char const*;

using nullptr_t = decltype(nullptr);

}

using namespace Ty;

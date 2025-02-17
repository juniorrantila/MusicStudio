#pragma once

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
#if __wasm32__ || _WIN32
typedef signed long long i64;
#else
typedef signed long i64;
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#if __wasm32__ || _WIN32
typedef unsigned long long u64;
#else
typedef unsigned long u64;
#endif

typedef signed __int128 i128;
typedef unsigned __int128 u128;

_Static_assert(sizeof(i8) == 1, "");
_Static_assert(sizeof(i16) == 2, "");
_Static_assert(sizeof(i32) == 4, "");
_Static_assert(sizeof(i64) == 8, "");

_Static_assert(sizeof(u8) == 1, "");
_Static_assert(sizeof(u16) == 2, "");
_Static_assert(sizeof(u32) == 4, "");
_Static_assert(sizeof(u64) == 8, "");

typedef __SIZE_TYPE__ usize;
#define unsigned signed
typedef __SIZE_TYPE__ isize;
#undef unsigned

typedef usize uptr;
typedef isize iptr;

_Static_assert(sizeof(uptr) == sizeof(void*), "");
_Static_assert(sizeof(iptr) == sizeof(void*), "");

typedef _Float16 f16;
_Static_assert(sizeof(f16) == 2, "");

typedef float f32;
_Static_assert(sizeof(f32) == 4, "");

typedef double f64;
_Static_assert(sizeof(f64) == 8, "");

typedef char const* c_string;

#ifdef __cplusplus
using nullptr_t = decltype(nullptr);
#else
#include <stdbool.h>
#endif

typedef i8 i8v2 __attribute__((ext_vector_type(2)));
typedef i8 i8v4 __attribute__((ext_vector_type(4)));
typedef u8 u8v2 __attribute__((ext_vector_type(2)));
typedef u8 u8v4 __attribute__((ext_vector_type(4)));

typedef i16 i16v2 __attribute__((ext_vector_type(2)));
typedef i16 i16v4 __attribute__((ext_vector_type(4)));
typedef u16 u16v2 __attribute__((ext_vector_type(2)));
typedef u16 u16v4 __attribute__((ext_vector_type(4)));

typedef i32 i32v2 __attribute__((ext_vector_type(2)));
typedef i32 i32v4 __attribute__((ext_vector_type(4)));
typedef u32 u32v2 __attribute__((ext_vector_type(2)));
typedef u32 u32v4 __attribute__((ext_vector_type(4)));

typedef i64 i64v2 __attribute__((ext_vector_type(2)));
typedef i64 i64v4 __attribute__((ext_vector_type(4)));
typedef u64 u64v2 __attribute__((ext_vector_type(2)));
typedef u64 u64v4 __attribute__((ext_vector_type(4)));

typedef isize isizev2 __attribute__((ext_vector_type(2)));
typedef isize isizev4 __attribute__((ext_vector_type(4)));
typedef usize usizev2 __attribute__((ext_vector_type(2)));
typedef usize usizev4 __attribute__((ext_vector_type(4)));

typedef iptr iptrv2 __attribute__((ext_vector_type(2)));
typedef iptr iptrv4 __attribute__((ext_vector_type(4)));
typedef uptr uptrv2 __attribute__((ext_vector_type(2)));
typedef uptr uptrv4 __attribute__((ext_vector_type(4)));

typedef f32 f32v2 __attribute__((ext_vector_type(2)));
typedef f32 f32v4 __attribute__((ext_vector_type(4)));
typedef f64 f64v2 __attribute__((ext_vector_type(2)));
typedef f64 f64v4 __attribute__((ext_vector_type(4)));


typedef double f64;
_Static_assert(sizeof(f64) == 8, "");

typedef bool bool2 __attribute__((ext_vector_type(2)));
typedef bool bool4 __attribute__((ext_vector_type(4)));

typedef bool bool256 __attribute__((ext_vector_type(256)));
_Static_assert(sizeof(bool256) == 32, "Let's not waste our bytes");

#define ty_offsetof(T, field) __builtin_offsetof(T, field)
#define ty_field_base(T, field, value) ((T*) (((u8*)(value)) - ty_offsetof(T, field)))

#ifdef __cplusplus
#define ty_static_assert static_assert
#else
#define ty_static_assert(expr, ...) _Static_assert((expr), __VA_ARGS__ "")
#endif

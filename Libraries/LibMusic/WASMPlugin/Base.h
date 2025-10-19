#pragma once

#ifdef __wasm__
#define MS_WASM(mod, name) [[clang::import_module(mod), clang::import_name(name)]]
#else
#define MS_WASM(mod, name)
#endif

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

typedef float f32;
typedef double f64;

typedef char const* c_string;

#ifdef __cplusplus
using nullptr_t = decltype(nullptr);
#else
#include <stdbool.h>
#endif

#pragma once
#ifndef __cplusplus
#if __STDC_VERSION__ < 202311L
#define alignas _Alignas
#define alignof _Alignof
#define bool _Bool
#define static_assert _Static_assert
#endif

#if __STDC_VERSION__ < 201711L
#define noreturn _Noreturn
#endif
#endif

#ifndef ty_offsetof
#define ty_offsetof __builtin_offsetof
#endif

typedef __INT8_TYPE__ i8;
typedef __INT16_TYPE__ i16;
typedef __INT32_TYPE__ i32;
// FIXME: typedef __INT64_TYPE__ i64;
#if __wasm32__ || _WIN32
typedef long long i64;
#else
typedef long i64;
#endif

typedef __UINT8_TYPE__ u8;
typedef __UINT16_TYPE__ u16;
typedef __UINT32_TYPE__ u32;

// FIXME: typedef __UINT64_TYPE__ u64;
#if __wasm32__ || _WIN32
typedef unsigned long long u64;
#else
typedef unsigned long u64;
#endif

typedef __SIZE_TYPE__ usize;
#define unsigned signed
typedef __SIZE_TYPE__ isize;
#undef unsigned

typedef usize uptr;
typedef isize iptr;

static_assert(sizeof(uptr) == sizeof(void*), "");
static_assert(sizeof(iptr) == sizeof(void*), "");

typedef float f32;
static_assert(sizeof(f32) == 4, "");

typedef double f64;
static_assert(sizeof(f64) == 8, "");

typedef char const* c_string;

#define field_base(T, field, value) ((T*) (((u8*)(value)) - ty_offsetof(T, field)))

#ifdef __cplusplus
#define C_API extern "C"
#else
#define C_API
#endif

#define RETURNS_SIZED_BY(size_parameter_index) \
    __attribute__((alloc_size(size_parameter_index)))

#define RETURNS_ALIGNED_BY(align_parameter_index) \
    __attribute__((alloc_size(align_parameter_index)))

#define RETURNS_SIZED_AND_ALIGNED_BY(size_parameter_index, align_parameter_index) \
    RETURNS_SIZED_BY(size_parameter_index) \
    RETURNS_ALIGNED_BY(align_parameter_index)

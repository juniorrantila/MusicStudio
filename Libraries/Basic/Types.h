#pragma once
#ifndef __cplusplus
#if __STDC_VERSION__ < 202311L
#define alignas _Alignas
#define alignof _Alignof
#define bool _Bool
#define static_assert(cond, ...) _Static_assert(cond, "" __VA_ARGS__)
#define thread_local _Thread_local
#define true ((bool)1)
#define false ((bool)0)
#endif

#if __STDC_VERSION__ < 201711L
#define noreturn _Noreturn
#endif
#endif

typedef __INT8_TYPE__ i8;
typedef __INT16_TYPE__ i16;
typedef __INT32_TYPE__ i32;
// typedef __INT64_TYPE__ i64;
typedef __int128 i128;

typedef __UINT8_TYPE__ u8;
typedef __UINT16_TYPE__ u16;
typedef __UINT32_TYPE__ u32;
// typedef __UINT64_TYPE__ u64;
typedef unsigned __int128 u128;

#if __wasm32__ || _WIN32
typedef signed long long i64;
#else
typedef signed long i64;
#endif

#if __wasm32__ || _WIN32
typedef unsigned long long u64;
#else
typedef unsigned long u64;
#endif

typedef __SIZE_TYPE__ uptr;
#define unsigned signed
typedef __SIZE_TYPE__ iptr;
#undef unsigned

static_assert(sizeof(uptr) == sizeof(void*));
static_assert(sizeof(iptr) == sizeof(void*));

typedef float f32;
static_assert(sizeof(f32) == 4);

typedef double f64;
static_assert(sizeof(f64) == 8);

typedef char const* c_string;

typedef f32 __attribute__((ext_vector_type(2))) v2;
static const v2 v2_zero = { 0, 0 };

typedef f32 __attribute__((ext_vector_type(4))) v4;
static const v4 v4_zero = { 0, 0, 0, 0 };

#ifdef __cplusplus
#define C_API extern "C"
#define C_EXTERN extern "C"
#define C_INLINE extern "C" inline
#else
#define C_API
#define C_EXTERN extern
#define C_INLINE static inline
#endif

C_INLINE v2 v2f(f32 x, f32 y)
{
    return (v2){x, y};
}

C_INLINE v4 v4f(f32 x, f32 y, f32 z, f32 w)
{
    return (v4){x, y, z, w};
}

#ifndef OFFSET_OF
#define OFFSET_OF __builtin_offsetof
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#ifndef BIT_ARRAY_SIZE
#define BIT_ARRAY_SIZE(a) (8 * ARRAY_SIZE(a))
#endif

#define FIELD_BASE(T, field, value) ((T*) (((u8*)(value)) - OFFSET_OF(T, field)))

#define SIZEOF_FIELD(Type, field) sizeof(((Type*)0)->field)

#if __has_feature(realtime_sanitizer)
C_API void __rtsan_disable(void);
C_API void __rtsan_enable(void);
#else
#define __rtsan_disable() do {} while(0)
#define __rtsan_enable() do {} while(0)
#endif

#define write_barrier() __sync_synchronize()

#ifdef __cplusplus
#define FILE_IS_CPP 1
#else
#define FILE_IS_CPP 0
#endif

#define VALIDATE_IS_CPP() static_assert(FILE_IS_CPP, "this file only works in C++ mode")

#ifdef __cplusplus
#define IF_CPP(...) __VA_ARGS__
#else
#define IF_CPP(...)
#endif

typedef __builtin_va_list va_list;
#ifndef va_arg
#define va_arg __builtin_va_arg
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#endif

#if __LITTLE_ENDIAN__
#define STRING_U64_2(s) 0    \
    | ((u64)s[7]) << (7 * 8) \
    | ((u64)s[6]) << (6 * 8) \
    | ((u64)s[5]) << (5 * 8) \
    | ((u64)s[4]) << (4 * 8) \
    | ((u64)s[3]) << (3 * 8) \
    | ((u64)s[2]) << (2 * 8) \
    | ((u64)s[1]) << (1 * 8) \
    | ((u64)s[0]) << (0 * 8)

#define CSTRING_U64_2(s) 0      \
    | ((u64)'\0') << (7 * 8)    \
    | ((u64)s[6]) << (6 * 8)    \
    | ((u64)s[5]) << (5 * 8)    \
    | ((u64)s[4]) << (4 * 8)    \
    | ((u64)s[3]) << (3 * 8)    \
    | ((u64)s[2]) << (2 * 8)    \
    | ((u64)s[1]) << (1 * 8)    \
    | ((u64)s[0]) << (0 * 8)

#elif __BIG_ENDIAN__

#define STRING_U64_2(s) 0    \
    | ((u64)s[0]) << (7 * 8) \
    | ((u64)s[1]) << (6 * 8) \
    | ((u64)s[2]) << (5 * 8) \
    | ((u64)s[3]) << (4 * 8) \
    | ((u64)s[4]) << (3 * 8) \
    | ((u64)s[5]) << (2 * 8) \
    | ((u64)s[6]) << (1 * 8) \
    | ((u64)s[7]) << (0 * 8)

#define CSTRING_U64_2(s) 0      \
    | ((u64)s[0]) << (7 * 8)    \
    | ((u64)s[1]) << (6 * 8)    \
    | ((u64)s[2]) << (5 * 8)    \
    | ((u64)s[3]) << (4 * 8)    \
    | ((u64)s[4]) << (3 * 8)    \
    | ((u64)s[5]) << (2 * 8)    \
    | ((u64)s[6]) << (1 * 8)    \
    | ((u64)'\0') << (0 * 8)
#else
#error "unknown endian"
#endif
#define STRING_U64(s) STRING_U64_2(s "\0\0\0\0\0\0\0\0")
#define STRING_U64(s) STRING_U64_2(s "\0\0\0\0\0\0\0\0")

#define CSTRING_U64(s) CSTRING_U64_2(s "\0\0\0\0\0\0\0\0")
#define CSTRING_U64(s) CSTRING_U64_2(s "\0\0\0\0\0\0\0\0")

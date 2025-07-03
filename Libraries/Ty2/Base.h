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

#ifndef ty_array_size
#define ty_array_size(a) (sizeof(a) / sizeof(a[0]))
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

typedef __attribute__((ext_vector_type(2))) f32 v2;
typedef __attribute__((ext_vector_type(4))) f32 v4;

#define field_base(T, field, value) ((T*) (((u8*)(value)) - ty_offsetof(T, field)))

#ifdef __cplusplus
#define C_API extern "C"
#define C_INLINE extern "C" inline
#else
#define C_API
#define C_INLINE static inline
#endif

typedef struct [[nodiscard]] { bool ok; } KSuccess;
C_INLINE KSuccess ksuccess() { return (KSuccess){true}; }
C_INLINE KSuccess kfail() { return (KSuccess){false}; }

#if __has_feature(realtime_sanitizer)
C_API void __rtsan_disable(void);
C_API void __rtsan_enable();
#else
#define __rtsan_disable() do {} while(0)
#define __rtsan_enable() do {} while(0)
#endif

#if __has_feature(address_sanitizer)
C_API void __asan_poison_memory_region(void const volatile *addr, usize size);
C_API void __asan_unpoison_memory_region(void const volatile *addr, usize size);
#else
#define __asan_poison_memory_region(addr, size) do { (void)(addr); (void)(size); } while(0)
#define __asan_unpoison_memory_region(addr, size) do { (void)(addr); (void)(size); } while(0)
#endif

#define mempoison(addr, size) __asan_poison_memory_region((addr), (size))
#define memunpoison(addr, size) __asan_unpoison_memory_region((addr), (size))

#define ty_write_barrier() __sync_synchronize()

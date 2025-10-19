#pragma once
#include <LibTy/Base.h>

#ifdef __cplusplus
#define INLINE static constexpr
#define CONSTEVAL consteval
#else
#define INLINE static inline
#define CONSTEVAL static inline
#endif

typedef struct Vec4f Vec4f;
typedef struct Vec2f {
    f32 x, y;

#ifdef __cplusplus
    constexpr bool is_inside(Vec4f) const;
#endif
} Vec2f;

INLINE Vec2f vec2f(f32 x, f32 y) { return (Vec2f){ x, y }; }
INLINE Vec2f vec2fs(f32 x) { return (Vec2f){ x, x }; }

#if __cplusplus
static constexpr Vec2f operator+(Vec2f a, Vec2f b) { return { a.x + b.x, a.y + b.y }; }
static constexpr Vec2f operator-(Vec2f a, Vec2f b) { return { a.x - b.x, a.y - b.y }; }
static constexpr Vec2f operator*(Vec2f a, Vec2f b) { return { a.x * b.x, a.y * b.y }; }
static constexpr Vec2f operator/(Vec2f a, Vec2f b) { return { a.x / b.x, a.y / b.y }; }

static constexpr Vec2f operator+(Vec2f a, f32 b) { return a + vec2fs(b); }
static constexpr Vec2f operator-(Vec2f a, f32 b) { return a - vec2fs(b); }
static constexpr Vec2f operator*(Vec2f a, f32 b) { return a * vec2fs(b); }
static constexpr Vec2f operator/(Vec2f a, f32 b) { return a / vec2fs(b); }

static constexpr Vec2f& operator+=(Vec2f& a, Vec2f b) { a = a + b; return a; }
static constexpr Vec2f& operator-=(Vec2f& a, Vec2f b) { a = a - b; return a; }
static constexpr Vec2f& operator*=(Vec2f& a, Vec2f b) { a = a * b; return a; }
static constexpr Vec2f& operator/=(Vec2f& a, Vec2f b) { a = a / b; return a; }
static constexpr Vec2f& operator/=(Vec2f& a, f32 b) { a = a / b; return a; }

static constexpr bool operator==(Vec2f a, Vec2f b) { return a.x == b.x & a.y == b.y; }
static constexpr bool operator!=(Vec2f a, Vec2f b) { return !(a == b); }

static constexpr bool operator<(Vec2f a, Vec2f b) { return a.x < b.x & a.y < b.y; }
static constexpr bool operator>(Vec2f a, Vec2f b) { return a.x > b.x & a.y > b.y; }

static constexpr bool operator<=(Vec2f a, Vec2f b) { return a.x <= b.x & a.y <= b.y; }
static constexpr bool operator>=(Vec2f a, Vec2f b) { return a.x >= b.x & a.y >= b.y; }
#endif

typedef struct Vec2i {
    i32 x, y;
} Vec2i;

#ifdef __cplusplus
static constexpr Vec2i vec2i(i32 x, i32 y) { return { x, y }; }
static constexpr Vec2i vec2is(i32 x) { return { x, x }; }
static constexpr Vec2i operator+(Vec2i a, Vec2i b) { return { a.x + b.x, a.y + b.y }; }
static constexpr Vec2i operator-(Vec2i a, Vec2i b) { return { a.x - b.x, a.y - b.y }; }
static constexpr Vec2i operator*(Vec2i a, Vec2i b) { return { a.x * b.x, a.y * b.y }; }
static constexpr Vec2i operator/(Vec2i a, Vec2i b) { return { a.x / b.x, a.y / b.y }; }
#endif

typedef struct Vec4f {
    union {
        struct {
            f32 x;
            f32 y;
            union {
                struct {
                    f32 z;
                    f32 w;
                };
                struct {
                    f32 width;
                    f32 height;
                };
            };
        };
        struct {
            f32 r, g, b, a;
        };
    };

#ifdef __cplusplus
    constexpr Vec2f start_point() const { return vec2f(x, y); }
    constexpr Vec2f size() const { return vec2f(width, height); }
    constexpr Vec2f end_point() const { return start_point() + size(); }
#endif
} Vec4f;

INLINE Vec4f vec4f(f32 x, f32 y, f32 z, f32 w) { return (Vec4f){ .x = x, .y = y, .z = z, .w = w }; }
INLINE Vec4f vec4fv(Vec2f xy, Vec2f zw) { return vec4f(xy.x, xy.y, zw.x, zw.y); }
INLINE Vec4f vec4fs(f32 x) { return vec4f(x, x, x, x); }

#ifdef __cplusplus
static constexpr Vec4f operator+(Vec4f a, Vec4f b) { return vec4f(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
static constexpr Vec4f operator-(Vec4f a, Vec4f b) { return vec4f(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
static constexpr Vec4f operator*(Vec4f a, Vec4f b) { return vec4f(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }
static constexpr Vec4f operator/(Vec4f a, Vec4f b) { return vec4f(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w); }

static constexpr Vec4f operator+(Vec4f a, f32 b) { return a + vec4fs(b); }
static constexpr Vec4f operator-(Vec4f a, f32 b) { return a - vec4fs(b); }
static constexpr Vec4f operator*(Vec4f a, f32 b) { return a * vec4fs(b); }
static constexpr Vec4f operator/(Vec4f a, f32 b) { return a / vec4fs(b); }

static constexpr Vec4f& operator+=(Vec4f& a, Vec4f b) { a = a + b; return a; }
static constexpr Vec4f& operator-=(Vec4f& a, Vec4f b) { a = a - b; return a; }
static constexpr Vec4f& operator*=(Vec4f& a, Vec4f b) { a = a * b; return a; }
static constexpr Vec4f& operator/=(Vec4f& a, Vec4f b) { a = a / b; return a; }
static constexpr Vec4f& operator/=(Vec4f& a, f32 b) { a = a / b; return a; }

constexpr bool Vec2f::is_inside(Vec4f rect) const
{
    return *this >= rect.start_point() && *this <= rect.end_point();
}
#endif

INLINE f32 lerpf(f32 a, f32 b, f32 t) { return a + (b - a) * t; }
INLINE Vec2f vec2f_lerp(Vec2f a, Vec2f b, Vec2f t) { return vec2f(lerpf(a.x, b.x, t.x), lerpf(a.y, b.y, t.y)); }

CONSTEVAL Vec4f hex_to_vec4f(u32 color)
{
    Vec4f result;
    u32 r = (color>>(3*8))&0xFF;
    u32 g = (color>>(2*8))&0xFF;
    u32 b = (color>>(1*8))&0xFF;
    u32 a = (color>>(0*8))&0xFF;
    result.x = r/255.0f;
    result.y = g/255.0f;
    result.z = b/255.0f;
    result.w = a/255.0f;
    return result;
}

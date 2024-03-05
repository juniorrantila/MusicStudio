#pragma once
#include <Ty/Base.h>

struct Vec4f;
struct Vec2f {
    f32 x, y;

    constexpr bool is_inside(Vec4f) const;
};

static constexpr Vec2f vec2f(f32 x, f32 y) { return { x, y }; }
static constexpr Vec2f vec2fs(f32 x) { return { x, x }; }

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

struct Vec2i {
    i32 x, y;
};

static constexpr Vec2i vec2i(i32 x, i32 y) { return { x, y }; }
static constexpr Vec2i vec2is(i32 x) { return { x, x }; }
static constexpr Vec2i operator+(Vec2i a, Vec2i b) { return { a.x + b.x, a.y + b.y }; }
static constexpr Vec2i operator-(Vec2i a, Vec2i b) { return { a.x - b.x, a.y - b.y }; }
static constexpr Vec2i operator*(Vec2i a, Vec2i b) { return { a.x * b.x, a.y * b.y }; }
static constexpr Vec2i operator/(Vec2i a, Vec2i b) { return { a.x / b.x, a.y / b.y }; }

struct Vec4f {
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

    constexpr Vec2f start_point() const { return vec2f(x, y); }
    constexpr Vec2f size() const { return vec2f(width, height); }
    constexpr Vec2f end_point() const { return start_point() + size(); }
};

static constexpr Vec4f vec4f(f32 x, f32 y, f32 z, f32 w) { return { .x = x, .y = y, .z = z, .w = w }; }
static constexpr Vec4f vec4fv(Vec2f xy, Vec2f zw) { return vec4f(xy.x, xy.y, zw.x, zw.y); }
static constexpr Vec4f vec4fs(f32 x) { return vec4f(x, x, x, x); }
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

static constexpr f32 lerpf(f32 a, f32 b, f32 t) { return a + (b - a) * t; }
static constexpr Vec2f vec2f_lerp(Vec2f a, Vec2f b, Vec2f t) { return vec2f(lerpf(a.x, b.x, t.x), lerpf(a.y, b.y, t.y)); }

consteval Vec4f hex_to_vec4f(u32 color)
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

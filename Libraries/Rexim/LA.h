#pragma once
#include <Ty/Base.h>

struct Vec2f {
    f32 x, y;
};

Vec2f vec2f(f32 x, f32 y);
Vec2f vec2fs(f32 x);
Vec2f vec2f_add(Vec2f a, Vec2f b);
Vec2f vec2f_sub(Vec2f a, Vec2f b);
Vec2f vec2f_mul(Vec2f a, Vec2f b);
Vec2f vec2f_mul3(Vec2f a, Vec2f b, Vec2f c);
Vec2f vec2f_div(Vec2f a, Vec2f b);

static inline Vec2f operator+(Vec2f a, Vec2f b) { return vec2f_add(a, b); }
static inline Vec2f operator-(Vec2f a, Vec2f b) { return vec2f_sub(a, b); }
static inline Vec2f operator*(Vec2f a, Vec2f b) { return vec2f_mul(a, b); }
static inline Vec2f operator/(Vec2f a, Vec2f b) { return vec2f_div(a, b); }

static inline Vec2f operator+(Vec2f a, f32 b) { return a + vec2fs(b); }
static inline Vec2f operator-(Vec2f a, f32 b) { return a - vec2fs(b); }
static inline Vec2f operator*(Vec2f a, f32 b) { return a * vec2fs(b); }
static inline Vec2f operator/(Vec2f a, f32 b) { return a / vec2fs(b); }

static inline Vec2f& operator+=(Vec2f& a, Vec2f b) { a = a + b; return a; }
static inline Vec2f& operator-=(Vec2f& a, Vec2f b) { a = a - b; return a; }
static inline Vec2f& operator*=(Vec2f& a, Vec2f b) { a = a * b; return a; }
static inline Vec2f& operator/=(Vec2f& a, Vec2f b) { a = a / b; return a; }
static inline Vec2f& operator/=(Vec2f& a, f32 b) { a = a / b; return a; }

struct Vec2i {
    i32 x, y;
};

Vec2i vec2i(i32 x, i32 y);
Vec2i vec2is(i32 x);
Vec2i vec2i_add(Vec2i a, Vec2i b);
Vec2i vec2i_sub(Vec2i a, Vec2i b);
Vec2i vec2i_mul(Vec2i a, Vec2i b);
Vec2i vec2i_mul3(Vec2i a, Vec2i b, Vec2i c);
Vec2i vec2i_div(Vec2i a, Vec2i b);

struct Vec4f {
    f32 x, y, z, w;
};

Vec4f vec4f(f32 x, f32 y, f32 z, f32 w);
Vec4f vec4fs(f32 x);
Vec4f vec4f_add(Vec4f a, Vec4f b);
Vec4f vec4f_sub(Vec4f a, Vec4f b);
Vec4f vec4f_mul(Vec4f a, Vec4f b);
Vec4f vec4f_div(Vec4f a, Vec4f b);
static inline Vec4f operator+(Vec4f a, Vec4f b) { return vec4f_add(a, b); }
static inline Vec4f operator-(Vec4f a, Vec4f b) { return vec4f_sub(a, b); }
static inline Vec4f operator*(Vec4f a, Vec4f b) { return vec4f_mul(a, b); }
static inline Vec4f operator/(Vec4f a, Vec4f b) { return vec4f_div(a, b); }

static inline Vec4f operator+(Vec4f a, f32 b) { return a + vec4fs(b); }
static inline Vec4f operator-(Vec4f a, f32 b) { return a - vec4fs(b); }
static inline Vec4f operator*(Vec4f a, f32 b) { return a * vec4fs(b); }
static inline Vec4f operator/(Vec4f a, f32 b) { return a / vec4fs(b); }

static inline Vec4f& operator+=(Vec4f& a, Vec4f b) { a = a + b; return a; }
static inline Vec4f& operator-=(Vec4f& a, Vec4f b) { a = a - b; return a; }
static inline Vec4f& operator*=(Vec4f& a, Vec4f b) { a = a * b; return a; }
static inline Vec4f& operator/=(Vec4f& a, Vec4f b) { a = a / b; return a; }
static inline Vec4f& operator/=(Vec4f& a, f32 b) { a = a / b; return a; }

f32 lerpf(f32 a, f32 b, f32 t);
Vec2f vec2f_lerp(Vec2f a, Vec2f b, Vec2f t);

Vec4f hex_to_vec4f(u32 color);

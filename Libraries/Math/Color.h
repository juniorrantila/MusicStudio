#pragma once

#ifdef __cplusplus
extern "C" {
#endif


typedef float f32;
typedef f32 f32v4 __attribute__((ext_vector_type(4)));
typedef struct { f32v4 value; } OKLAB;
typedef struct { f32v4 value; } OKLCH;
typedef struct { f32v4 value; } RGBA;
typedef struct { f32v4 value; } LinearRGBA;


OKLAB oklab(f32 lightness, f32 a, f32 b, f32 alpha);
OKLCH oklch(f32 lightness, f32 chroma, f32 hue, f32 alpha);
RGBA rgba(f32 r, f32 g, f32 b, f32 a);
LinearRGBA linear_rgba(f32 r, f32 g, f32 b, f32 a);


RGBA rgba_oklab(f32 lightness, f32 a, f32 b, f32 alpha);
RGBA rgba_oklch(f32 lightness, f32 chroma, f32 hue, f32 alpha);


LinearRGBA linear_oklab(f32 lightness, f32 a, f32 b, f32 alpha);
LinearRGBA linear_oklch(f32 lightness, f32 chroma, f32 hue, f32 alpha);


RGBA rgba_from_oklab(OKLAB);
RGBA rgba_from_oklch(OKLCH);
RGBA rgba_from_linear(LinearRGBA);
RGBA rgba_brighten(RGBA, f32 percent);
RGBA rgba_darken(RGBA, f32 percent);
RGBA rgba_shade(RGBA, f32 percent);


LinearRGBA linear_from_oklab(OKLAB);
LinearRGBA linear_from_oklch(OKLCH);
LinearRGBA linear_from_rgba(RGBA);
LinearRGBA linear_brighten(LinearRGBA, f32 percent);
LinearRGBA linear_darken(LinearRGBA, f32 percent);
LinearRGBA linear_shade(LinearRGBA, f32 percent);


OKLAB oklab_from_oklch(OKLCH);
OKLAB oklab_from_linear(LinearRGBA);
OKLAB oklab_from_rgba(RGBA);
OKLAB oklab_brighten(OKLAB, f32 percent);
OKLAB oklab_darken(OKLAB, f32 percent);
OKLAB oklab_shade(OKLAB, f32 percent);


OKLCH oklch_from_oklab(OKLAB);
OKLCH oklch_from_linear(LinearRGBA);
OKLCH oklch_from_rgba(RGBA);
OKLCH oklch_brighten(OKLCH, f32 percent);
OKLCH oklch_darken(OKLCH, f32 percent);
OKLCH oklch_shade(OKLCH, f32 percent);


#ifdef __cplusplus
}
#endif

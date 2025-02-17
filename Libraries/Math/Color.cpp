#include "./Color.h"

#include "./Math.h"


static f32 linear_component_from_rgba(f32 x);
static f32 rgba_component_from_linear(f32 x);


OKLAB oklab(f32 lightness, f32 a, f32 b, f32 alpha) { return (OKLAB){{lightness, a, b, alpha}}; }
OKLCH oklch(f32 lightness, f32 chroma, f32 hue, f32 alpha) { return (OKLCH){{lightness, chroma, hue, alpha}}; }
RGBA rgba(f32 r, f32 g, f32 b, f32 a) { return (RGBA){{r, g, b, a}}; }
LinearRGBA linear_rgba(f32 r, f32 g, f32 b, f32 a) { return (LinearRGBA){{r, g, b, a}}; }


RGBA rgba_oklab(f32 lightness, f32 a, f32 b, f32 alpha) { return rgba_from_oklab(oklab(lightness, a, b, alpha)); }
RGBA rgba_oklch(f32 lightness, f32 chroma, f32 hue, f32 alpha) { return rgba_from_oklch(oklch(lightness, chroma, hue, alpha)); }


LinearRGBA linear_oklab(f32 lightness, f32 a, f32 b, f32 alpha) { return linear_from_rgba(rgba_oklab(lightness, a, b, alpha)); }
LinearRGBA linear_oklch(f32 lightness, f32 chroma, f32 hue, f32 alpha) { return linear_from_rgba(rgba_oklch(lightness, chroma, hue, alpha)); }


RGBA rgba_from_oklch(OKLCH c) { return rgba_from_oklab(oklab_from_oklch(c)); }
RGBA rgba_from_linear(LinearRGBA c)
{
    return rgba(
        rgba_component_from_linear(c.value.r),
        rgba_component_from_linear(c.value.g),
        rgba_component_from_linear(c.value.b),
        c.value.a
    );
}
RGBA rgba_from_oklab(OKLAB ok) 
{
    f32v4 c = ok.value;
    f32 l_ = c.x + 0.3963377774f * c.y + 0.2158037573f * c.z;
    f32 m_ = c.x - 0.1055613458f * c.y - 0.0638541728f * c.z;
    f32 s_ = c.x - 0.0894841775f * c.y - 1.2914855480f * c.z;

    f32 l = l_ * l_ * l_;
    f32 m = m_ * m_ * m_;
    f32 s = s_ * s_ * s_;

    return rgba(
		+4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
		-1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
		-0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s,
        c.w
    );
}
RGBA rgba_brighten(RGBA c, f32 percent) { return rgba_shade(c, 1.0f + percent); }
RGBA rgba_darken(RGBA c, f32 percent) { return rgba_shade(c, 1.0f - percent); }
RGBA rgba_shade(RGBA c, f32 percent) { return rgba_from_linear(linear_shade(linear_from_rgba(c), percent)); }


LinearRGBA linear_from_oklab(OKLAB c) { return linear_from_rgba(rgba_from_oklab(c)); }
LinearRGBA linear_from_oklch(OKLCH c) { return linear_from_rgba(rgba_from_oklch(c)); }
LinearRGBA linear_from_rgba(RGBA c)
{
    return linear_rgba(
        linear_component_from_rgba(c.value.r),
        linear_component_from_rgba(c.value.g),
        linear_component_from_rgba(c.value.b),
        c.value.a
    );
}
LinearRGBA linear_brighten(LinearRGBA c, f32 percent) { return linear_shade(c, 1.0f + percent); }
LinearRGBA linear_darken(LinearRGBA c, f32 percent) { return linear_shade(c, 1.0f - percent); }
LinearRGBA linear_shade(LinearRGBA c, f32 percent) { return linear_from_oklab(oklab_shade(oklab_from_linear(c), percent)); }


OKLAB oklab_from_oklch(OKLCH c)
{
    return oklab(
		c.value.x,
		c.value.y * math_cosf(c.value.z),
		c.value.y * math_sinf(c.value.z),
        c.value.w
    );
}
OKLAB oklab_from_linear(LinearRGBA linear)
{
    f32v4 c = linear.value;
    f32 l = math_cbrtf(0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b);
	f32 m = math_cbrtf(0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b);
	f32 s = math_cbrtf(0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b);

    return oklab(
        0.2104542553f*l + 0.7936177850f*m - 0.0040720468f*s,
        1.9779984951f*l - 2.4285922050f*m + 0.4505937099f*s,
        0.0259040371f*l + 0.7827717662f*m - 0.8086757660f*s,
        c.a
    );
}
OKLAB oklab_from_rgba(RGBA c) { return oklab_from_linear(linear_from_rgba(c)); }
OKLAB oklab_brighten(OKLAB c, f32 percent) { return oklab_shade(c, 1.0f + percent); }
OKLAB oklab_darken(OKLAB c, f32 percent) { return oklab_shade(c, 1.0f - percent); }
OKLAB oklab_shade(OKLAB c, f32 percent)
{
    c.value.x *= percent;
    return c;
}
OKLCH oklch_from_oklab(OKLAB oklab)
{
    f32v4 c = oklab.value;
    return oklch(
		c.x,
        math_sqrtf(c.y * c.y + c.z * c.z),
        math_atan2f(c.z, c.y),
        c.w
	);
}
OKLCH oklch_from_linear(LinearRGBA c) { return oklch_from_oklab(oklab_from_linear(c)); }
OKLCH oklch_from_rgba(RGBA c) { return oklch_from_linear(linear_from_rgba(c)); }
OKLCH oklch_brighten(OKLCH c, f32 percent) { return oklch_shade(c, 1.0f + percent); }
OKLCH oklch_darken(OKLCH c, f32 percent) { return oklch_shade(c, 1.0f - percent); }
OKLCH oklch_shade(OKLCH c, f32 percent)
{
    c.value.x *= percent;
    return c;
}


static f32 linear_component_from_rgba(f32 x)
{
	if (x >= 0.04045f) {
        return math_powf((x + 0.055f) * (1.0f / 1.0055f), 2.0f);
	}
	return x * (1.0f / 12.92f);
}

static f32 rgba_component_from_linear(f32 x)
{
    if (x >= 0.0031308f) {
		return 1.055f * math_powf(x, 1.0f / 2.4f) - 0.055f;
	}
	return 12.92f * x;
}

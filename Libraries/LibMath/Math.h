#pragma once

#ifdef __cplusplus
extern "C" {
#endif

inline float math_abs_f32(float x) { return x < 0.0f ? -x : x; }
inline double math_abs_f64(double x) { return x < 0.0 ? -x : x; }
inline float math_floor_f32(float x) { return (float)(long long)x; }
inline double math_floor_f64(double x) { return (double)(long long)x; }
inline float math_ceil_f32(float x) { return x == math_floor_f32(x) ? x : math_floor_f32(x) + 1.0f; }
inline double math_ceil_f64(double x) { return x == math_floor_f64(x) ? x : math_floor_f64(x) + 1.0; }
inline float math_frac_f32(float x) { return x - math_floor_f32(x); }
inline double math_frac_f64(double x) { return x - math_floor_f64(x); }

inline float math_cos_turns_f32(float t)
{
    t -= 0.25f + math_floor_f32(t + 0.25f);
    t *= 16.0f * (math_abs_f32(t) - 0.5f);
    t += 0.225f * t * (math_abs_f32(t) - 1.0f);
    return t;
}

inline double math_cos_turns_f64(double t)
{
    t -= 0.25 + math_floor_f64(t + 0.25);
    t *= 16.0 * (math_abs_f64(t) - 0.5);
    t += 0.225 * t * (math_abs_f64(t) - 1.0);
    return t;
}

inline float math_sin_turns_f32(float t) { return math_cos_turns_f32(t + 0.5f); }
inline double math_sin_turns_f64(double t) { return math_cos_turns_f64(t + 0.5); }

inline float math_mod_f32(float x, float y) { return __builtin_fmodf(x, y); }
inline double math_mod_f64(double x, double y) { return __builtin_fmod(x, y); }

inline float math_round_f32(float t) { return math_abs_f32(math_frac_f32(t)) < 0.5f ? math_floor_f32(t) : math_floor_f32(t + 1.0f); }
inline double math_round_f64(double t) { return math_abs_f64(math_frac_f64(t)) < 0.5 ? math_floor_f64(t) : math_floor_f64(t + 1.0); }

inline float math_lerp_f32(float v0, float v1, float t) { return (v0 * (1.0f - t) + v1 * t); }
inline double math_lerp_f64(double v0, double v1, double t) { return (v0 * (1.0 - t) + v1 * t); }

inline float math_sqrtf(float v) { return __builtin_sqrtf(v); }
inline double math_sqrt(double v) { return __builtin_sqrt(v); }

inline float math_cbrtf(float x) { return __builtin_cbrtf(x); }
inline double math_cbrt(double x) { return __builtin_cbrt(x); }

inline float math_sinf(float x) { return __builtin_sinf(x); }
inline double math_sin(double x) { return __builtin_sin(x); }

inline float math_cosf(float x) { return __builtin_cosf(x); }
inline double math_cos(double x) { return __builtin_cos(x); }

inline float math_atanf(float x) { return __builtin_atanf(x); }
inline double math_atan(double x) { return __builtin_atan(x); }

inline float math_atan2f(float y, float x) { return __builtin_atan2f(y, x); }
inline double math_atan2(double y, double x) { return __builtin_atan2(y, x); }

inline float math_powf(float x, float y) { return __builtin_powf(x, y); }
inline double math_pow(double x, double y) { return __builtin_pow(x, y); }

#ifdef __cplusplus
}
#endif

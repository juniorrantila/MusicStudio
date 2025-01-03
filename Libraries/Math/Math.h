#pragma once

#ifdef __cplusplus
extern "C" {
#endif

inline float math_abs_f32(float x) { return x < 0.0f ? -x : x; }
inline double math_abs_f64(double x) { return x < 0.0 ? -x : x; }
inline float math_floor_f32(float x) { return (float)(long long)x; }
inline double math_floor_f64(double x) { return (double)(long long)x; }
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

#ifdef __cplusplus
}
#endif

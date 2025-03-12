#include "./Transcoder.h"

#include <string.h>

C_API f64* au_interlace_f64(Allocator* gpa, f64 const* in, usize frames, usize channels)
{
    f64* out = gpa->alloc<f64>(frames * channels);
    if (!out) return nullptr;
    for (usize channel = 0; channel < channels; channel++) {
        for (usize frame = 0; frame < frames; frame++) {
            out[frame * channels + channel] = in[channel * frames + frame];
        }
    }
    return out;
}

C_API f64* au_interlace_f64_from_f32(Allocator* gpa, f32 const* in, usize frames, usize channels)
{
    f64* out = gpa->alloc<f64>(frames * channels);
    if (!out) return nullptr;
    for (usize channel = 0; channel < channels; channel++) {
        for (usize frame = 0; frame < frames; frame++) {
            out[frame * channels + channel] = in[channel * frames + frame];
        }
    }
    return out;
}

C_API f64* au_deinterlace_f64(Allocator* gpa, f64 const* in, usize frames, usize channels)
{
    f64* out = gpa->alloc<f64>(frames * channels);
    if (!out) return nullptr;
    for (usize channel = 0; channel < channels; channel++) {
        for (usize frame = 0; frame < frames; frame++) {
            out[channel * frames + frame] = in[frame * channels + channel];
        }
    }
    return out;
}

C_API f32* au_deinterlace_f32_from_f64(Allocator* gpa, f64 const* in, usize frames, usize channels)
{
    f32* out = gpa->alloc<f32>(frames * channels);
    if (!out) return nullptr;
    for (usize channel = 0; channel < channels; channel++) {
        for (usize frame = 0; frame < frames; frame++) {
            out[channel * frames + frame] = (f32)in[frame * channels + channel];
        }
    }
    return out;
}

C_API f64** au_shallow_split_channels_f64(Allocator* arena, usize min_splits, f64* in, usize frames, usize channels)
{
    usize max_channels = channels > min_splits ? channels : min_splits;
    f64** out = arena->alloc<f64*>(max_channels);
    if (!out) return nullptr;
    for (usize i = 0; i < min_splits; i++) {
        out[i] = arena->alloc<f64>(frames);
        if (!out[i]) return nullptr;
        memset(out[i], 0, sizeof(f64) * frames);
    }
    for (usize channel = 0; channel < channels; channel++) {
        out[channel] = &in[frames * channel];
    }
    return out;
}

C_API f32** au_shallow_split_channels_f32(Allocator* arena, usize min_splits, f32* in, usize frames, usize channels)
{
    usize max_channels = channels > min_splits ? channels : min_splits;
    f32** out = arena->alloc<f32*>(max_channels);
    if (!out) return nullptr;
    for (usize i = 0; i < min_splits; i++) {
        out[i] = arena->alloc<f32>(frames);
        if (!out[i]) return nullptr;
        memset(out[i], 0, sizeof(f32) * frames);
    }
    for (usize channel = 0; channel < channels; channel++) {
        out[channel] = &in[frames * channel];
    }
    return out;
}

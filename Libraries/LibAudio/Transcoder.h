#pragma once
#include <Basic/Types.h>
#include <Basic/Forward.h>

C_API f64* au_interlace_f64(Allocator*, f64 const* in, u64 frames, u64 channels);
C_API f64* au_interlace_f64_from_f32(Allocator*, f32 const* in, u64 frames, u64 channels);

C_API f64* au_deinterlace_f64(Allocator*, f64 const* in, u64 frames, u64 channels);
C_API f32* au_deinterlace_f32_from_f64(Allocator*, f64 const* in, u64 frames, u64 channels);

C_API f64** au_shallow_split_channels_f64(Allocator* arena, u64 min_splits, f64* in, u64 frames, u64 channels);
C_API f32** au_shallow_split_channels_f32(Allocator* arena, u64 min_splits, f32* in, u64 frames, u64 channels);

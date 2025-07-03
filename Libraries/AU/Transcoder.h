#pragma once
#include <Ty/Base.h>
#include <Ty2/Allocator.h>

C_API f64* au_interlace_f64(Allocator*, f64 const* in, usize frames, usize channels);
C_API f64* au_interlace_f64_from_f32(Allocator*, f32 const* in, usize frames, usize channels);

C_API f64* au_deinterlace_f64(Allocator*, f64 const* in, usize frames, usize channels);
C_API f32* au_deinterlace_f32_from_f64(Allocator*, f64 const* in, usize frames, usize channels);

C_API f64** au_shallow_split_channels_f64(Allocator* arena, usize min_splits, f64* in, usize frames, usize channels);
C_API f32** au_shallow_split_channels_f32(Allocator* arena, usize min_splits, f32* in, usize frames, usize channels);

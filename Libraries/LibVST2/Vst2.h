#pragma once
#include <LibTy/Base.h>

#include "./VSTHostEvent.h"
#include "./VSTPluginEvent.h"

typedef enum {
    VSTEffect_HasEditor           = 1 << 0,
    VSTEffect__HasClip            = 1 << 1, // Deprecated.
    VSTEffect__HasVU              = 1 << 2, // Deprecated.
    VSTEffect__CanMono            = 1 << 3, // Deprecated.
    VSTEffect_CanF32Replacing     = 1 << 4,
    VSTEffect_ProgramChunks       = 1 << 5,
    VSTEffect_IsSynth             = 1 << 8,
    VSTEffect_IsSilentWhenStopped = 1 << 9,
    VSTEffect__IsAsync            = 1 << 10, // Deprecated.
    VSTEffect__HasBuffer          = 1 << 11, // Deprecated.
    VSTEffect_CanF64Replacing     = 1 << 12,
} VSTEffectOption;

typedef struct VSTEffect VSTEffect;
typedef struct VSTEffect {
	i32 vst_magic;

	VSTPluginDispatchFunc dispatch;
	void(*_process)(VSTEffect*, f32 const* const* in, f32* const* out, i32 frames); // Deprecated.
	void(*set_parameter)(VSTEffect*, i32 index, f32 value);
	f32(*get_parameter)(VSTEffect*, i32 index);

	i32 preset_count;
	i32 parameter_count;
	i32 input_count;
	i32 output_count;

	i32 effect_options; // VSTEffectOption

	iptr reserved1;
	iptr reserved2;

	i32 initial_delay;
 
	i32 _real_qualities; // Deprecated.
	i32 _off_qualities;  // Deprecated.
	f32 _io_ratio;       // Deprecated.

	void* user_data;
	void* host_data;

	u32 author_id; // Registered at Steinberg
	u32 version; // Maybe called product_version

	void(*process_f32)(VSTEffect*, f32 const* const* in, f32* const* out, i32 frames);;
	void(*process_f64)(VSTEffect*, f64 const* const* in, f64* const* out, i32 frames);;

	u8 future[56];
} VSTEffect;

#if _WIN32
#define VST_EXPORT __declspec(dllexport) C_API
#else
#define VST_EXPORT C_API
#endif

typedef VSTEffect*(*VSTPluginMainFunc)(VSTHostDispatchFunc);
VST_EXPORT VSTEffect* VSTPluginMain(VSTHostDispatchFunc);

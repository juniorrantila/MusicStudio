#pragma once
#include "./Host.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char const ms_plugin_name[] __asm__("ms_plugin_name");
extern void ms_plugin_process_f64(MSPluginHost const*, f64* out, f64 const* in, u32 frames);
extern void ms_plugin_process_f32(MSPluginHost const*, f32* out, f32 const* in, u32 frames);
extern void ms_plugin_init(MSPluginHost const* host);
extern void ms_plugin_deinit(MSPluginHost const* host);

#ifdef __cplusplus
}
#endif

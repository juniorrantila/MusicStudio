#pragma once
#include "./Host.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char const native_plugin_name[];
extern void native_plugin_process_f32(NativePluginHost const*, f32* out, f32 const* in, u32 frames);
extern void native_plugin_init(NativePluginHost const* host);
extern void native_plugin_deinit(NativePluginHost const* host);

#ifdef __cplusplus
}
#endif

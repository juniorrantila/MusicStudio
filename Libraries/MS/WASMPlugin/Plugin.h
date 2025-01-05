#pragma once
#include "./Base.h"

#ifdef __cplusplus
extern "C" {
#endif

MS_WASM("ms", "get_sample_rate") extern u32 ms_get_sample_rate(void);
MS_WASM("ms", "get_channels") extern u32 ms_get_channels(void);

MS_WASM("ms", "log_info")
[[gnu::format(printf, 1, 0)]]
extern void ms_info(c_string format, ...);

MS_WASM("ms", "log_debug")
[[gnu::format(printf, 1, 0)]]
extern void ms_debug(c_string format, ...);

MS_WASM("ms", "log_error")
[[gnu::format(printf, 1, 0)]]
extern void ms_error(c_string format, ...);

MS_WASM("ms", "log_fatal")
[[noreturn, gnu::format(printf, 1, 0)]]
extern void ms_fatal(c_string format, ...);

MS_WASM("ms", "time_f32")
extern f32 ms_time_f32(void);

MS_WASM("ms", "time_f64")
extern f32 ms_time_f64(void);

MS_WASM("ms", "store")
extern i64 ms_store(void const* key, u32 key_size, void const* value, u32 value_size);

MS_WASM("ms", "stores")
extern i64 ms_stores(c_string key, void const* value, u32 value_size);

MS_WASM("ms", "fetch")
extern i64 ms_fetch(void const* key, u32 key_size, void* value, u32 value_size);

MS_WASM("ms", "heap_base")
void* ms_heap_base(void);

MS_WASM("ms", "heap_end")
void* ms_heap_end(void);

// Defined by the plugin.

// This is used to categorize the plugin in the MusicStudio UI.
typedef char const MSPluginKind[];
#define MSPluginKind_Effect     ("com.music-studio.effect")
#define MSPluginKind_Generator  ("com.music-studio.generator")
#define MSPluginKind_Instrument ("com.music-studio.instrument")

// Version needs to be different if a memcpy of memory
// isn't sufficient for initializing a plugin between runs.
// NOTE: May not be 0.
extern u32 const ms_plugin_version __asm__("ms_plugin_version");
extern char const ms_plugin_id[] __asm__("ms_plugin_id");
extern char const ms_plugin_name[] __asm__("ms_plugin_name");
extern MSPluginKind const ms_plugin_kind __asm__("ms_plugin_kind");

void ms_plugin_init(u32 plugin_version) __asm__("ms_plugin_init");
void ms_plugin_deinit(void) __asm__("ms_plugin_deinit");

void ms_plugin_draw_ui(void) __asm__("ms_plugin_draw_ui");
void ms_plugin_process_f32(f32* out, f32 const* in, u32 frames, u32 channels) __asm__("ms_plugin_process_f32");
void ms_plugin_process_f64(f64* out, f64 const* in, u32 frames, u32 channels) __asm__("ms_plugin_process_f64");


#ifdef __cplusplus
}
#endif

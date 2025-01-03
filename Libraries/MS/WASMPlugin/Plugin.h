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
[[gnu::format(printf, 1, 0)]]
extern void ms_fatal(c_string format, ...);

// Defined by the plugin.
extern const char ms_plugin_name[] __asm__("ms_plugin_name");
void ms_plugin_draw_ui(void) __asm__("ms_plugin_draw_ui");
void ms_plugin_process_f64(f64* out, f64* in, u32 frames) __asm__("ms_plugin_process_f64");
void ms_plugin_process_f32(f32* out, f32* in, u32 frames) __asm__("ms_plugin_process_f32");
void ms_plugin_main(void) __asm__("ms_plugin_main");

#ifdef __cplusplus
}
#endif

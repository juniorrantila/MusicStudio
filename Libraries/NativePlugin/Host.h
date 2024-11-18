#pragma once
#include <Ty/Base.h>

typedef struct NativePluginHost {
    u64 size;

    void (*log_err)(struct NativePluginHost const* host, c_string fmt, ...);
    void (*log_info)(struct NativePluginHost const* host, c_string fmt, ...);
    void (*log_debug)(struct NativePluginHost const* host, c_string fmt, ...);

    u32 (*get_sample_rate)(struct NativePluginHost const* host);
    u32 (*get_channels)(struct NativePluginHost const* host);
} NativePluginHost;

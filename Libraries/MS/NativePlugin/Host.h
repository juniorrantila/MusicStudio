#pragma once
#include <Ty/Base.h>

typedef struct MSPluginHost MSPluginHost;
struct MSPluginHost {
    u64 size;

    void* user;

    void (*log_debug)(MSPluginHost const* host, c_string fmt, ...);
    void (*log_err)(MSPluginHost const* host, c_string fmt, ...);
    void (*log_info)(MSPluginHost const* host, c_string fmt, ...);

    u32 (*get_sample_rate)(MSPluginHost const* host);
    u32 (*get_channels)(MSPluginHost const* host);
};

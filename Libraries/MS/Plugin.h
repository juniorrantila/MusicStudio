#pragma once
#include <Ty/ErrorOr.h>
#include <NativePlugin/Host.h>
#include <NativePlugin/Plugin.h>
#include <Library/Library.h>

namespace MS {

struct PluginManager;
struct Plugin {
    Plugin(usize id, PluginManager const* manager, Library* library);

    void init();
    void deinit();
    void process_f32(f32* out, f32 const* in, u32 frames);

private:
    struct Client {
        decltype(native_plugin_name)* name = nullptr;
        decltype(native_plugin_process_f32)* process_f32 = nullptr;
        decltype(native_plugin_init)* init = nullptr;
        decltype(native_plugin_deinit)* deinit = nullptr;
    };

    Plugin(usize id, PluginManager const* manager, Client client);

    [[gnu::format(printf, 2, 3)]]
    static void log_err(NativePluginHost const* host, c_string fmt, ...);

    [[gnu::format(printf, 2, 3)]]
    static void log_info(NativePluginHost const* host, c_string fmt, ...);

    [[gnu::format(printf, 2, 3)]]
    static void log_debug(NativePluginHost const* host, c_string fmt, ...);

    static u32 get_sample_rate(NativePluginHost const* host);
    static u32 get_channels(NativePluginHost const* host);

    c_string name() const;

    NativePluginHost m_host {
        .size = sizeof(NativePluginHost),
        .log_err = log_err,
        .log_info= log_info,
        .log_debug = log_debug,
        .get_sample_rate = get_sample_rate,
        .get_channels = get_channels,
    };
    usize m_id { 0 };
    PluginManager const* m_manager { nullptr };
    Client m_client {};
};

}

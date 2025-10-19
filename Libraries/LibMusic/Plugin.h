#pragma once
#include <LibTy/ErrorOr.h>
#include <LibCore/Library.h>

#include "./NativePlugin/Host.h"
#include "./NativePlugin/Plugin.h"

namespace MS {

struct PluginManager;
struct Plugin {
    Plugin(usize id, PluginManager const* manager, Library* library);

    void init();
    void deinit();
    void process_f32(f32* out, f32 const* in, u32 frames);
    void process_f64(f64* out, f64 const* in, u32 frames);

    bool can_process_f32() const { return m_client.process_f32 != nullptr; }
    bool can_process_f64() const { return m_client.process_f64 != nullptr; }

private:
    struct Client {
        decltype(ms_plugin_name)* name = nullptr;
        decltype(ms_plugin_process_f32)* process_f32 = nullptr;
        decltype(ms_plugin_process_f64)* process_f64 = nullptr;
        decltype(ms_plugin_init)* init = nullptr;
        decltype(ms_plugin_deinit)* deinit = nullptr;
    };

    Plugin(usize id, PluginManager const* manager, Client client);

    [[gnu::format(printf, 2, 3)]]
    static void log_err(MSPluginHost const* host, c_string fmt, ...);

    [[gnu::format(printf, 2, 3)]]
    static void log_info(MSPluginHost const* host, c_string fmt, ...);

    [[gnu::format(printf, 2, 3)]]
    static void log_debug(MSPluginHost const* host, c_string fmt, ...);

    static u32 get_sample_rate(MSPluginHost const* host);
    static u32 get_channels(MSPluginHost const* host);

    c_string name() const;

    MSPluginHost m_host {
        .size = sizeof(MSPluginHost),
        .user = 0,
        .log_debug = log_debug,
        .log_err = log_err,
        .log_info = log_info,
        .get_sample_rate = get_sample_rate,
        .get_channels = get_channels,
    };
    usize m_id { 0 };
    PluginManager const* m_manager { nullptr };
    Client m_client {};
};

}

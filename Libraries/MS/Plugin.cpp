#include "./Plugin.h"

#include "./PluginManager.h"

#include <stdarg.h>
#include <stdio.h>

namespace MS {

Plugin::Plugin(usize id, MS::PluginManager const* manager, Library* lib)
    : Plugin(id, manager, Client{
        .name = (decltype(Client::name))optional([&] {
            return library_get_symbol(lib, "_native_plugin_name");
        }).or_else([&] {
            return library_get_symbol(lib, "native_plugin_name");
        }),
        .process_f32 = (decltype(Client::process_f32))optional([&] {
            return library_get_symbol(lib, "_native_plugin_process_f32");
        }).or_else([&] {
            return library_get_symbol(lib, "native_plugin_process_f32");
        }),
        .init = (decltype(Client::init))optional([&] {
            return library_get_symbol(lib, "_native_plugin_init");
        }).or_else([&] {
            return library_get_symbol(lib, "native_plugin_init");
        }),
        .deinit = (decltype(Client::deinit))optional([&] {
            return library_get_symbol(lib, "_native_plugin_deinit");
        }).or_else([&] {
            return library_get_symbol(lib, "native_plugin_deinit");
        }),
    })
{
}

Plugin::Plugin(usize id, MS::PluginManager const* manager, Client client)
    : m_id(id)
    , m_manager(manager)
    , m_client(client)
{
    static_assert(__builtin_offsetof(Plugin, m_host) == 0);
}

void Plugin::init()
{
    if (m_client.init) {
        m_client.init(&m_host);
    }
}

void Plugin::deinit()
{
    if (m_client.deinit) {
        m_client.deinit(&m_host);
    }
}

void Plugin::process_f32(f32* out, f32 const* in, u32 frames)
{
    if (m_client.process_f32) {
        m_client.process_f32(&m_host, out, in, frames);
    }
}

void Plugin::log_err(NativePluginHost const* host, c_string fmt, ...)
{
    auto* plugin = (Plugin const*)host;
    va_list args;
    va_start(args, fmt);
    printf("[%zu:%s] error: ", plugin->m_id, plugin->name());
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

void Plugin::log_info(NativePluginHost const* host, c_string fmt, ...)
{
    auto* plugin = (Plugin const*)host;
    va_list args;
    va_start(args, fmt);
    printf("[%zu:%s] info: ", plugin->m_id, plugin->name());
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

void Plugin::log_debug(NativePluginHost const* host, c_string fmt, ...)
{
    auto* plugin = (Plugin const*)host;
    va_list args;
    va_start(args, fmt);
    printf("[%zu:%s] debug: ", plugin->m_id, plugin->name());
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

u32 Plugin::get_sample_rate(NativePluginHost const* host)
{
    auto* plugin = (Plugin const*)host;
    return plugin->m_manager->project->sample_rate;
}

u32 Plugin::get_channels(NativePluginHost const* host)
{
    auto* plugin = (Plugin const*)host;
    return plugin->m_manager->project->channels;
}

c_string Plugin::name() const
{
    if (m_client.name)
        return *m_client.name;
    return "<no name>";
}

}

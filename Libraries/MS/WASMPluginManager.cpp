#include "./WASMPluginManager.h"

#include "./Project.h"

namespace MS {

ErrorOr<Id<WASMPlugin>> WASMPluginManager::add_plugin(StringView path)
{
    return TRY(m_plugins.append(TRY(WASMPlugin::create(this, path))));
}

ErrorOr<void> WASMPluginManager::link()
{
    for (auto& plugin : m_plugins) {
        TRY(plugin.link());
    }
    return {};
}

ErrorOr<void> WASMPluginManager::init()
{
    for (auto& plugin : m_plugins) {
        TRY(plugin.init());
    }
    return {};
}

ErrorOr<void> WASMPluginManager::deinit()
{
    for (auto& plugin : m_plugins) {
        TRY(plugin.deinit());
    }
    return {};
}

u32 WASMPluginManager::sample_rate() const
{
    return m_project->sample_rate;
}

u32 WASMPluginManager::channel_count() const
{
    return m_project->channels;
}

}

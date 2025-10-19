#include "./WASMPluginManager.h"

#include "./Project.h"

#include <Basic/PageAllocator.h>
#include <Basic/Defer.h>

#include <LibTy/StringBuffer.h>
#include <LibCore/MappedFile.h>

namespace MS {

ErrorOr<Id<WASMPlugin>> WASMPluginManager::add_plugin(StringView path)
{
    auto file_id = TRY(m_plugin_files.append(TRY(Core::MappedFile::open(path))));
    auto name = TRY(path.split_on('/')).last();
    return TRY(add_plugin(TRY(WASMPlugin::create(this, name, m_plugin_files[file_id].view().as_bytes()))));
}

ErrorOr<Id<WASMPlugin>> WASMPluginManager::add_plugin(WASMPlugin&& plugin)
{
    auto id = TRY(m_plugins.append(move(plugin)));
    m_plugins[id].manager = this;
    return id;
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

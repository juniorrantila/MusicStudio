#include "./WASMPluginManager.h"

namespace MS {

ErrorOr<void> WASMPluginManager::add_plugin(WASMPlugin&& plugin)
{
    TRY(wasm_plugins.append(move(plugin)));
    return {};
}

ErrorOr<void> WASMPluginManager::link()
{
    for (auto& plugin : wasm_plugins) {
        TRY(plugin.link());
    }
    return {};
}

ErrorOr<void> WASMPluginManager::run()
{
    for (auto& plugin : wasm_plugins) {
        TRY(plugin.run());
    }
    return {};
}

}

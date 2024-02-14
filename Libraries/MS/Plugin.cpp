#include "Plugin.h"

#include <Ty/ScopeGuard.h>

namespace MS {

ErrorOr<Plugin> Plugin::create_from(const char *path)
{
    auto library = TRY(Core::Library::open_local(path));

    using Signature = Vst::PluginMainSignature;
    auto create_plugin = library.fetch_symbol<Signature>("VSTPluginMain");
    if (create_plugin.is_error()) {
        create_plugin = library.fetch_symbol<Signature>("main");
        if (create_plugin.is_error())
            return Error::from_string_literal("could not find entry point to plugin (VSTPluginMain or main)");
    }

    auto plugin = create_plugin.release_value()(Host::dispatch);
    if (!plugin)
        return Error::from_string_literal("could not instantiate plugin");

    if (plugin->vst_magic != 'VstP')
        return Error::from_string_literal("plugin header magic number is not 'VstP'");

    // if (!plugin->init())
    //     return Error::from_string_literal("could not initialize plugin");
    (void)plugin->init();
    ScopeGuard deinit_plugin = [&] {
        (void)plugin->deinit();
    };

    (void)plugin->pause();

    auto host = TRY(Host::create(plugin));
    plugin->host_data = host;

    deinit_plugin.disarm();

    return Plugin {
        move(library),
        plugin,
        host
    };
}

void Plugin::destroy() const
{
    if (is_valid()) {
        host->destroy();
        (void)vst->deinit();
        const_cast<Plugin*>(this)->invalidate();
    }
}

}

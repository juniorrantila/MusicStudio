#include "./VstPlugin.h"

#include <Basic/Defer.h>
#include <LibTy/Try.h>

namespace MS {

ErrorOr<Plugin> Plugin::create_from(const char *path)
{
    auto* library = library_create_from_path(path);

    using Signature = Vst::PluginMainSignature;
    auto create_plugin = (Signature)library_get_symbol(library, "VSTPluginMain");
    if (!create_plugin) {
        create_plugin = (Signature)library_get_symbol(library, "main");
        if (!create_plugin)
            return Error::from_string_literal("could not find entry point to plugin (VSTPluginMain or main)");
    }

    auto* plugin = create_plugin(Host::dispatch);
    if (!plugin)
        return Error::from_string_literal("could not instantiate plugin");

    if (plugin->vst_magic != 'VstP')
        return Error::from_string_literal("plugin header magic number is not 'VstP'");

    // if (!plugin->init())
    //     return Error::from_string_literal("could not initialize plugin");
    (void)plugin->init();
    Defer deinit_plugin = [&] {
        (void)plugin->deinit();
    };

    (void)plugin->pause();

    auto* host = TRY(Host::create(plugin));
    plugin->host_data = host;

    deinit_plugin.disarm();

    return Plugin {
        move(library),
        plugin,
        host,
    };
}

void Plugin::destroy() const
{
    if (is_valid()) {
        host->destroy();
        (void)vst->deinit();
        library_destroy(plugin_library);
        const_cast<Plugin*>(this)->invalidate();
    }
}

}

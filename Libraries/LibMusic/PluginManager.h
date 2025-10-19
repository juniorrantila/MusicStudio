#pragma once
#include "./Plugin.h"

#include "./Project.h"

#include <LibTy/SmallMap.h>

#include <LibTy/Id.h>
#include <LibTy/Vector.h>
#include <LibCore/Library.h>

namespace MS {

struct PluginManager {
    static ErrorOr<PluginManager> create(Allocator* arena)
    {
        auto manager = PluginManager();
        manager.arena = arena;

        auto plugin_count = 1024;

        auto* plugins = arena->alloc<Plugin>(plugin_count);
        if (!plugins) return Error::from_string_literal("could not allocate plugins");
        manager.plugins = View(plugins, plugin_count);

        auto* libraries = arena->alloc<Library*>(plugin_count);
        if (!plugins) return Error::from_string_literal("could not allocate libraries");
        manager.libraries = View(libraries, plugin_count);

        return manager;
    }

    PluginManager init(MS::Project const* project)&&
    {
        this->project = project;
        return static_cast<PluginManager&&>(*this);
    }

    void deinit();

    ErrorOr<Id<Plugin>> instantiate(c_string path);

    Plugin& operator[](Id<Plugin> id)
    {
        return plugins[id.raw()];
    }

    SmallMap<StringView, View<u8>> mapped_files {};
    Allocator* arena = nullptr;
    usize plugin_count = 0;
    View<Plugin> plugins {};
    View<Library*> libraries {};
    MS::Project const* project { nullptr };
};

}

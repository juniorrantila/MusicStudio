#pragma once
#include "./Plugin.h"

#include "./Project.h"

#include <Ty/ArenaAllocator.h>
#include <Ty/SmallMap.h>

#include <Ty/Id.h>
#include <Ty/Vector.h>
#include <Library/Library.h>

namespace MS {

struct PluginManager {
    static ErrorOr<PluginManager> create(Allocator* arena)
    {
        auto manager = PluginManager();
        manager.arena = arena;
        manager.plugins = TRY(arena->alloc<Plugin>(1024));
        manager.libraries = TRY(arena->alloc<Library*>(1024));
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

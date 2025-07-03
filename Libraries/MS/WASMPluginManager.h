#pragma once
#include "./WASMPlugin.h"

#include <Ty/Vector.h>

namespace MS {

struct Project;
struct WASMPluginManager {
    WASMPluginManager(Project* project)
        : m_project(project)
    {
    }

    ErrorOr<Id<WASMPlugin>> add_plugin(StringView path);
    ErrorOr<void> link();
    ErrorOr<void> init();
    ErrorOr<void> deinit();

    u32 sample_rate() const;
    u32 channel_count() const;

    View<WASMPlugin const> plugins() const { return m_plugins.view(); }
    WASMPlugin const* plugin(Id<WASMPlugin> id) const { return &m_plugins[id]; }

private:
    Project* m_project;
    Vector<WASMPlugin> m_plugins {};
};

}

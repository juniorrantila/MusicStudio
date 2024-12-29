#pragma once
#include "./WASMPlugin.h"

#include <Ty/Vector.h>

namespace MS {

struct WASMPluginManager {
    Vector<WASMPlugin> wasm_plugins;

    ErrorOr<void> add_plugin(WASMPlugin&& plugin);
    ErrorOr<void> link();
    ErrorOr<void> run();
};

}

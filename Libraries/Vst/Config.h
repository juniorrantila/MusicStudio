#pragma once
#include <JR/Types.h>
#include <Vst/Vst.h>
#include <Vst/AEffect.h>

namespace Vst {

struct Config {
    char const* plugin_name;
    char const* author_name;
    char const* product_name;
    PluginCategory category;
    PluginFlagsUnderlying flags;
    u32 author_id;
    u32 vst_version { 2400 };
    u32 plugin_version;
    u32 number_of_inputs;
    u32 number_of_outputs;
    u32 number_of_presets;
    u32 number_of_parameters;
    u32 initial_delay;
};

}

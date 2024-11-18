#include "./AEffect.h"
#include "./AudioPlugin.h"
#include "./ChunkType.h"
#include "./Config.h"
#include "./KnobMode.h"
#include "./Opcodes.h"
#include "./Rectangle.h"
#include "./Vst.h"

#define LOG_IF(cond, ...) ((void)cond)
#define LOG(...)

namespace Vst {

static iptr plugin_callback_(Effect* effect,
    PluginOpcode opcode, i32 index,
    iptr value, void* ptr,
    f32 opt)
{
    constexpr auto log_plugin_callback = true;
    auto* plugin = (AudioPlugin*)effect->plugin;

    if (opcode == PluginOpcode::Create) {
        LOG_IF(log_plugin_callback, "%s()", plugin_opcode_string(opcode));
        return 1;
    }

    if (opcode == PluginOpcode::Destroy) {
        LOG_IF(log_plugin_callback, "%s()", plugin_opcode_string(opcode));
        delete plugin;
        delete effect;
        return 1;
    }

    if (opcode == PluginOpcode::ProcessEvents)
        return plugin->dispatch(opcode, index, value, ptr, opt);
    if (opcode == PluginOpcode::EditorIdle)
        return plugin->dispatch(opcode, index, value, ptr, opt);
    if (opcode == PluginOpcode::GetPresetNumber)
        return plugin->dispatch(opcode, index, value, ptr, opt);
    if (opcode == PluginOpcode::GetMidiKeyName)
        return plugin->dispatch(opcode, index, value, ptr, opt);

    auto const* name = plugin_opcode_string(opcode);
    auto parens = !index && !value && !ptr && opt == 0 ? "()" : "";
    if (name)
        LOG_IF(log_plugin_callback, "%s%s", name, parens);
    else
        LOG_IF(log_plugin_callback, "%d%s", opcode, parens);
    if (parens[0] == '\0') {
        if (index)
            LOG_IF(log_plugin_callback, "    index: %d", index);
        if (value)
            LOG_IF(log_plugin_callback, "    value: %ld", value);
        if (ptr)
            LOG_IF(log_plugin_callback, "    ptr:   0x%p", ptr);
        if (opt)
            LOG_IF(log_plugin_callback, "    opt:   %f", opt);
    }
    return plugin->dispatch(opcode, index, value, ptr, opt);
}

static void process_f32_(Effect* effect, f32 const* const* inputs,
    f32* const* outputs, i32 samples)
{
    auto* plugin = (AudioPlugin*)effect->plugin;
    plugin->process_f32(inputs, outputs, samples);
}

static void process_f64_(Effect* effect, f64 const* const* inputs,
    f64* const* outputs, i32 samples)
{
    auto* plugin = (AudioPlugin*)effect->plugin;
    plugin->process_f64(inputs, outputs, samples);
}

static void set_parameter_(Effect* effect, i32 index, f32 value)
{
    auto* plugin = (AudioPlugin*)effect->plugin;
    return plugin->set_parameter(index, value);
}

static f32 get_parameter_(Effect* effect, i32 index)
{
    auto* plugin = (AudioPlugin*)effect->plugin;
    return plugin->parameter(index);
}

Effect::Effect(AudioPlugin* plugin)
    : plugin_callback(plugin_callback_)
    , _process(process_f32_)
    , set_parameter(set_parameter_)
    , get_parameter(get_parameter_)
    , plugin(plugin)
    , process_f32(process_f32_)
    , process_f64(process_f64_)
{
    auto config = plugin->config();
    number_of_presets = config.number_of_presets;
    number_of_parameters = config.number_of_parameters;
    number_of_inputs = config.number_of_inputs;
    number_of_outputs = config.number_of_outputs;
    flags = config.flags;
    author_id = config.author_id;
    version = config.plugin_version;
}

}

















#include <Vst/AEffect.h>
#include <Vst/Opcodes.h>
#include <Vst/PluginData.h>
#include <Vst/Rectangle.h>
#include <Vst/Vst.h>
#include <Vst/CanDo.h>

namespace Vst {

bool Effect::init()
{
    return dispatch(PluginOpcode::Create);
}

bool Effect::deinit()
{
    return dispatch(PluginOpcode::Destroy);
}

bool Effect::set_preset_number(i32 value)
{
    return dispatch(PluginOpcode::SetPresetNumber, value);
}

i32 Effect::preset_number()
{
    return dispatch(PluginOpcode::GetPresetNumber);
}

bool Effect::set_preset_name(char const* name)
{
    return dispatch(PluginOpcode::SetPresetName, 0, 0, (void*)name, 0.0);
}

char const* Effect::preset_name()
{
    return (char const*)dispatch(PluginOpcode::GetPresetName);
}

char const* Effect::parameter_label(char* name, i32 id)
{
    return (char const*)dispatch(PluginOpcode::GetParameterLabel, id, 0, name);
}

char const* Effect::parameter_display(char* name, i32 id)
{
    return (char const*)dispatch(PluginOpcode::GetParameterDisplay, id, 0, name);
}

char const* Effect::parameter_name(char* name, i32 id)
{
    dispatch(PluginOpcode::GetParameterName, id, 0, name);
    return name;
}

bool Effect::set_sample_rate(f32 value)
{
    return dispatch(PluginOpcode::SetSampleRate, 0, value, nullptr, value);
}

bool Effect::set_block_size(i32 value)
{
    return dispatch(PluginOpcode::SetBlockSize, 0, value);
}

bool Effect::pause()
{
    return dispatch(PluginOpcode::MainsChanged, 0, 0);
}

bool Effect::resume()
{
    return dispatch(PluginOpcode::MainsChanged, 0, 1);
}

Optional<Rectangle> Effect::editor_rectangle()
{
    Rectangle* rectangle = nullptr;
    if (dispatch(PluginOpcode::GetEditorRectangle, 0, 0, &rectangle))
        return *rectangle;
    return {};
}

bool Effect::set_editor_rectangle(Rectangle const* rect)
{
    return dispatch(PluginOpcode::VendorSpecific, (i32)ExtensionVendor::Prosonus, (iptr)ProsonusPluginOpcode::EditSetRect, const_cast<Rectangle*>(rect));
}

bool Effect::open_editor(void* window)
{
    return dispatch(PluginOpcode::CreateEditor, 0, 0, window);
}

bool Effect::close_editor()
{
    return dispatch(PluginOpcode::DestroyEditor);
}

bool Effect::editor_idle()
{
    return dispatch(PluginOpcode::EditorIdle);
}

void* Effect::chunk(ChunkType type)
{
    void* data = nullptr;
    dispatch(PluginOpcode::GetChunk, (i32)type, 0, &data);
    return data;
}

bool Effect::set_chunk(ChunkType type, void* data, i32 data_size)
{
    return dispatch(PluginOpcode::SetChunk, (i32)type, data_size, data);
}

bool Effect::process_events(Events* events)
{
    
    return dispatch(PluginOpcode::ProcessEvents, 0, 0, events);
}

bool Effect::parameter_can_be_automated(i32 id)
{
    return dispatch(PluginOpcode::CanBeAutomated, id);
}

bool Effect::string_to_parameter(i32 id, char const* value)
{
    return dispatch(PluginOpcode::StringToParameter, id, 0, (void*)value);
}

char const* Effect::preset_name_from_id(i32 id)
{
#if 0
    auto plugin_data = ((PluginData*)host_data);
    if (plugin_data->name_of_preset_with_id_has_value(id))
        return plugin_data->preset_names[id];

    if (!plugin_data->name_of_preset_with_id_name_is_allocated(id))
        plugin_data->allocate_space_for_name_of_preset_with_id(id);
    auto name = plugin_data->preset_names[id];
#endif
    static thread_local char name[MAX_NAME_LENGTH];
    dispatch(PluginOpcode::GetPresetNameIndexed, id, 0, name);
    return name;
}

// FIXME: Handle fail.
PinProperties Effect::input_properties(i32 input_id) const
{
    PinProperties properties {};
    dispatch(PluginOpcode::GetInputProperties, input_id, 0, &properties);
    return properties;
}

// FIXME: Handle fail.
PinProperties Effect::output_properties(i32 output_id) const
{
    PinProperties properties {};
    dispatch(PluginOpcode::GetOutputProperties, output_id, 0, &properties);
    return properties;
}

PluginCategory Effect::category()
{
    return (PluginCategory)dispatch(PluginOpcode::GetPlugCategory);
}

bool Effect::offline_notify(AudioFile* files, i32 files_size, bool start)
{
    return dispatch(PluginOpcode::OfflineNotify, files_size, start, files);
}

bool Effect::offline_prepare(OfflineTask* tasks, i32 tasks_size)
{
    return dispatch(PluginOpcode::OfflinePrepare, tasks_size, 0, tasks);
}

bool Effect::offline_run(OfflineTask* tasks, i32 tasks_size)
{
    return dispatch(PluginOpcode::OfflinePrepare, tasks_size, 0, tasks);
}

bool Effect::process_var_io(ProcessVarIo* io)
{
    return dispatch(PluginOpcode::ProcessVarIo, 0, 0, io);
}

bool Effect::set_speaker_arrangement(SpeakerArrangement* input_type, SpeakerArrangement* output_type)
{
    return dispatch(PluginOpcode::SetSpeakerArrangement, 0,
                    (iptr)input_type, output_type);
}

bool Effect::set_bypass(bool value)
{
    return dispatch(PluginOpcode::SetBypass, 0, value);
}

// FIXME: Might not be able to be cached,
//        if this is meant for shell plugins.
char const* Effect::name()
{
#if 0
    auto plugin_data = ((PluginData*)host_data);
    auto name = plugin_data->plugin_name;
    if (plugin_data->plugin_name_is_set())
        return name;
#endif
    static thread_local char name[MAX_NAME_LENGTH];
    if (!dispatch(PluginOpcode::GetPluginName, 0, 0, name))
        return nullptr;
    return name;
}

char const* Effect::author()
{
#if 0
    auto plugin_data = ((PluginData*)host_data);
    auto name = plugin_data->author_name;
    if (plugin_data->author_name_is_set())
        return name;
#endif
    static thread_local char name[MAX_NAME_LENGTH];
    if (!dispatch(PluginOpcode::GetAuthorName, 0, 0, name))
        return nullptr;
    return name;
}

char const* Effect::product_name()
{
#if 0
    auto plugin_data = ((PluginData*)host_data);
    auto name = plugin_data->product_name;
    if (plugin_data->author_name_is_set())
        return name;
#endif
    static thread_local char name[MAX_NAME_LENGTH];
    if (!dispatch(PluginOpcode::GetProductName, 0, 0, name))
        return nullptr;
    return name;
}

u32 Effect::product_version()
{
    return dispatch(PluginOpcode::GetProductVersion);
}

iptr Effect::vendor_specific(i32 index, iptr value,
        void* ptr, f32 opt)
{
    return dispatch(PluginOpcode::GetProductVersion, index, value, ptr,
            opt);
}

CanDo Effect::can_do(c_string thing)
{
    return (CanDo)dispatch(PluginOpcode::CanDo, 0, 0, (void*)thing, 0);
}

i32 Effect::tail_size()
{
    return dispatch(PluginOpcode::GetTailSize);
}

// FIXME: Handle error.
ParameterProperties Effect::parameter_properties(i32 parameter_id)
{
    ParameterProperties properties {};
    dispatch(PluginOpcode::GetParameterProperties, parameter_id, 0, &properties);
    return properties;
}

u32 Effect::vst_version()
{
    return dispatch(PluginOpcode::GetVstVersion);
}

bool Effect::key_down(char character, VirtualKey key, ModifierKeyUnderlying modifiers)
{
    return dispatch(PluginOpcode::NotifyKeyDown, character, (iptr)key, nullptr, modifiers);
}

bool Effect::key_up(char character, VirtualKey key, ModifierKeyUnderlying modifiers)
{
    return dispatch(PluginOpcode::NotifyKeyUp, character, (iptr)key, nullptr, modifiers);
}

bool Effect::set_knob_mode(KnobMode mode)
{
    return dispatch(PluginOpcode::SetEditKnobMode, 0, (iptr)mode);
}

bool Effect::begin_set_program()
{
    return dispatch(PluginOpcode::BeginSetProgram);
}

bool Effect::end_set_program()
{
    return dispatch(PluginOpcode::EndSetProgram);
}

bool Effect::start_process()
{
    return dispatch(PluginOpcode::StartProcess);
}

bool Effect::stop_process()
{
    return dispatch(PluginOpcode::StopProcess);
}

bool Effect::set_total_samples_to_process(i32 value)
{
    return dispatch(PluginOpcode::SetTotalSampleToProcess, 0, value);
}

bool Effect::set_pan_saw(PanLaw pan_law, f32 gain)
{
    return dispatch(PluginOpcode::SetPanLaw, 0, (iptr)pan_law, 0, gain);
}

bool Effect::set_process_precision(Precision value)
{
    return dispatch(PluginOpcode::SetProcessPrecision, 0, (iptr)value);
}

u8 Effect::midi_input_channels_size()
{
    return dispatch(PluginOpcode::GetNumberOfMidiInputChannels);
}

u8 Effect::midi_output_channels_size()
{
    return dispatch(PluginOpcode::GetNumberOfMidiOutputChannels);
}

bool Effect::set_tempo(i32 time_per_beat_in_samples, f32 tempo)
{
    return dispatch(PluginOpcode::SetTempo, time_per_beat_in_samples, 0, 0, tempo);
}

iptr Effect::dispatch(PluginOpcode opcode, i32 index,
                  iptr value, void* ptr, f32 opt)
{
    return plugin_callback(this, opcode, index, value, ptr, opt);
}

iptr Effect::dispatch(PluginOpcode opcode, i32 index,
                  iptr value, void* ptr, f32 opt) const
{
    return plugin_callback(const_cast<Effect*>(this), opcode, index, value, ptr, opt);
}

}

#pragma once
#include <Ty/Base.h>
#include <Ty/Optional.h>
#include <Vst/AEffect.h>
#include <Vst/ChunkType.h>
#include <Vst/KnobMode.h>
#include <Vst/Opcodes.h>
#include <Vst/Precision.h>

#if _WIN32
#    define VST_EXPORT __declspec(dllexport)
#else
#    define VST_EXPORT
#endif


namespace Vst {

struct Effect;

using HostCallback = iptr(*)(Effect* effect,
                                 HostOpcode opcode,
                                 i32 index,
                                 iptr value,
                                 void* ptr, f32 opt);
using PluginCallback = iptr(*)(Effect* effect,
                                   PluginOpcode opcode,
                                   i32 index,
                                   iptr value,
                                   void* ptr,
                                   f32 opt);
using ProcessF32Proc = void(*)(Effect* effect,
                               f32 const* const* inputs,
                               f32* const* outputs,
                               i32 samples);
using ProcessF64Proc = void (*)(Effect* effect,
                                f64 const* const* inputs,
                                f64* const* outputs,
                                i32 samples);
using SetParameterProc = void(*)(Effect* effect,
                                 i32 index,
                                 f32 parameter);
using GetParameterProc = float(*)(Effect* effect,
                                  i32 index);

using PluginFlagsUnderlying = i32;
enum class PluginFlags : PluginFlagsUnderlying {
    HasEditor           = 1 << 0,
    _HasClip            = 1 << 1, // Deprecated.
    _HasVU              = 1 << 2, // Deprecated.
    _CanMono            = 1 << 3, // Deprecated.
    CanF32Replacing     = 1 << 4,
    ProgramChunks       = 1 << 5,
    IsSynth             = 1 << 8,
    IsSilentWhenStopped = 1 << 9,
    _IsAsync            = 1 << 10, // Deprecated.
    _HasBuffer          = 1 << 11, // Deprecated.
    CanF64Replacing     = 1 << 12,
};

struct AudioPlugin;
struct Events;
struct Rectangle;
struct CanDo;
struct ProcessVarIo;
struct Effect 
{
    Effect(AudioPlugin* plugin);

	i32 vst_magic { 'VstP' };

	PluginCallback plugin_callback;
	ProcessF32Proc _process; // Deprecated.
	SetParameterProc set_parameter;
	GetParameterProc get_parameter;

	i32 number_of_presets;
	i32 number_of_parameters;
	i32 number_of_inputs;
	i32 number_of_outputs;

	PluginFlagsUnderlying flags;

	iptr reserved1 { 0 };
	iptr reserved2 { 0 };

	i32 initial_delay;
 
	i32 _realQualities { 0 }; // Deprecated.
	i32 _offQualities { 0 };  // Deprecated.
	f32 _ioRatio { 1.0 };     // Deprecated.

	void* plugin;
	void* host_data { nullptr };

	u32 author_id; // Registered at Steinberg
                   // 3rd party support Web.
	u32 version; // Maybe called product_version
                        // because of shell plugins.

	ProcessF32Proc process_f32;
	ProcessF64Proc process_f64;

	u8 future[56] {};

	bool has_editor() const
    {
        return flags & (i32)PluginFlags::HasEditor;
    }
    bool supports_f32() const
    {
        return flags & (i32)PluginFlags::CanF32Replacing;
    }
    bool supports_f64() const
    {
        return flags & (i32)PluginFlags::CanF64Replacing;
    }
    bool uses_program_chunks() const
    {
        return flags & (i32)PluginFlags::ProgramChunks;
    }
    bool is_synth() const
    {
        return flags & (i32)PluginFlags::IsSynth;
    }
    bool is_silent_when_stopped() const
    {
        return flags & (i32)PluginFlags::IsSilentWhenStopped;
    }

    [[nodiscard]] bool init();
    [[nodiscard]] bool deinit();
    [[nodiscard]] bool set_preset_number(i32 value);
    i32 preset_number();
    [[nodiscard]] bool set_preset_name(char const* name);
    char const* preset_name();
    char const* parameter_label();
    char const* parameter_display();
    char const* parameter_name();
    [[nodiscard]] bool set_sample_rate(f32 value);
    [[nodiscard]] bool set_block_size(i32 value);
    [[nodiscard]] bool pause();
    [[nodiscard]] bool resume();
    Optional<Rectangle> editor_rectangle();
    [[nodiscard]] bool open_editor(void* window);
    [[nodiscard]] bool close_editor();
    [[nodiscard]] bool editor_idle();
    void* chunk(ChunkType type);
    [[nodiscard]] bool set_chunk(ChunkType type, void* data, i32 data_size);
    [[nodiscard]] bool process_events(Events*);
    [[nodiscard]] bool parameter_can_be_automated(i32 id);
    [[nodiscard]] bool string_to_parameter(i32 id, char const* value);
    char const* preset_name_from_id(i32 id);
    // FIXME: Handle fail.
    PinProperties input_properties(i32 input_id) const;
    // FIXME: Handle fail.
    PinProperties output_properties(i32 output_id) const;
    PluginCategory category();
    [[nodiscard]] bool offline_notify(AudioFile* files, i32 files_size, bool start);
    [[nodiscard]] bool offline_prepare(OfflineTask* tasks, i32 tasks_size);
    [[nodiscard]] bool offline_run(OfflineTask* tasks, i32 tasks_size);

    [[nodiscard]] bool process_var_io(ProcessVarIo*);
    [[nodiscard]] bool set_speaker_arrangement(SpeakerArrangement* input_type, SpeakerArrangement* output_type);
    [[nodiscard]] bool set_bypass(bool value);
    char const* name();
    char const* author();
    char const* product_name();
    u32 product_version();
    iptr vendor_specific(i32 index, iptr value, void* ptr, f32 opt);
    CanDo can_do(char const* thing);
    i32 tail_size();
    ParameterProperties parameter_properties(i32 parameter_id);
    u32 vst_version();
    bool key_down(char character, VirtualKey key, ModifierKeyUnderlying modifiers);
    [[nodiscard]] bool key_up(char character, VirtualKey key, ModifierKeyUnderlying modifiers);
    [[nodiscard]] bool set_knob_mode(KnobMode mode);
#if 0
    GetMidiProgramName,         // index: MIDI channel.
                                // ptr: MidiProgramName*.
                                // return value: number of used programs, 0 if unsupported.
    GetCurrentMidiProgram,      // index: MIDI channel.
                                // ptr: MidiProgramName*.
                                // return value: index of current program.
    GetMidiProgramCategory,     // index: MIDI channel.
                                // ptr: MidiProgramCategory*.
                                // return value: number of used categories, 0 if unsupported.
    HasMidiProgramsChanged,     // index: MIDI channel.
                                // return value: 1 if the MidiProgramName or MidiKeyName have changed.
    GetMidiKeyName,             // index: MIDI channel.
                                // ptr: MidiKeyName*.
                                // return value: true if supported, false otherwise.

#endif
    [[nodiscard]] bool begin_set_program();
    [[nodiscard]] bool end_set_program();
    // SpeakerArrangement speaker_arrangement(SpeakerArrangement*)
    // ShellGetNextPlugin,         // ptr: plug-in name char[kVstMaxProductStrLen].
                                // return value: next plugin's unique_id.

    [[nodiscard]] bool start_process();
    [[nodiscard]] bool stop_process();
    [[nodiscard]] bool set_total_samples_to_process(i32 value);
    [[nodiscard]] bool set_pan_saw(PanLaw pan_law, f32 gain);

#if 0
    BeginLoadBank,              // ptr: VstPatchChunkInfo*.
                                // return value:
                                //  * -1: bank can't be loaded,
                                //  *  1: bank can be loaded,
                                //  *  0: unsupported.
    BeginLoadProgram,           // ptr: VstPatchChunkInfo*.
                                // return value:
                                //  * -1: program can't be loaded,
                                //  *  1: program can be loaded,
                                //  *  0: unsupported  

#endif
    [[nodiscard]] bool set_process_precision(Precision value);
    u8 midi_input_channels_size();
    u8 midi_output_channels_size();

    [[nodiscard]] bool set_tempo(i32 time_per_beat_in_samples, f32 tempo);

private:
    iptr dispatch(PluginOpcode opcode, i32 index = 0, iptr value = 0,
            void* ptr = nullptr, f32 opt = 0.0);

    iptr dispatch(PluginOpcode opcode, i32 index = 0, iptr value = 0,
            void* ptr = nullptr, f32 opt = 0.0) const;
};

using PluginMainSignature = Vst::Effect*(*)(Vst::HostCallback);

}

VST_EXPORT extern "C" Vst::Effect* VSTPluginMain(Vst::HostCallback);

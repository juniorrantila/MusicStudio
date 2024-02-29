#pragma once
#include <Ty/Base.h>

#include "./AEffect.h"
#include "./Vst.h"
#include "./KnobMode.h"
#include "./Precision.h"
#include "./Rectangle.h"
#include "./ChunkType.h"
#include "./CanDo.h"
#include "./Host.h"
#include "./Config.h"
#ifdef __APPLE__
#include <objc/runtime.h>
#endif

namespace Vst {

#ifdef __APPLE__
using NativeHandle = id;
#else
using NativeHandle = void*;
#endif

struct AudioPlugin {
    AudioPlugin() = default;
    virtual ~AudioPlugin() {}

    virtual Config config() const = 0;

    virtual bool open_editor(NativeHandle window_handle)
    {
        (void)window_handle;
        return false;
    }

    virtual void editor_loop() {}
    virtual void close_editor() {}
    virtual bool set_editor_dpi(f32)
    {
        return false;
    }

    [[nodiscard]] virtual bool resume() { return true; }
    [[nodiscard]] virtual bool suspend() { return true; }

    [[nodiscard]] virtual bool start_process()
    {
        return true;
    }
    [[nodiscard]] virtual bool stop_process()
    {
        return true;
    }

    virtual void process_f32(f32 const* const* inputs,
                             f32* const* outputs,
                             i32 samples)
    {
        (void)inputs;
        (void)outputs;
        (void)samples;
    }

    virtual void process_f64(f64 const* const* inputs,
                             f64* const* outputs,
                             i32 samples)
    {
        (void)inputs;
        (void)outputs;
        (void)samples;
    }

    [[nodiscard]] virtual bool set_current_preset(i32 id)
    {
        (void)id;
        return false;
    }

    virtual i32 current_preset() const
    {
        return -1;
    }

    [[nodiscard]] virtual bool set_preset_name(char const* new_name)
    {
        (void)new_name;
        return false;
    }

    virtual char const* preset_name() const
    {
        return nullptr;
    }

    virtual char const* parameter_label(i32 parameter_id) const
    {
        (void)parameter_id;
        return nullptr;
    }

    virtual char const* parameter_display(i32 parameter_id) const
    {
        (void)parameter_id;
        return nullptr;
    }

    virtual char const* parameter_name(i32 parameter_id) const
    {
        (void)parameter_id;
        return "noname";
    }

    [[nodiscard]] virtual bool set_sample_rate(f32 value) = 0;
    [[nodiscard]] virtual bool set_block_size(i32 value) = 0;

    // bool get_chunk(ChunkType type);
    [[nodiscard]] virtual bool set_chunk(ChunkType type, i32 id, void* value)
    {
        (void)type;
        (void)id;
        (void)value;
        return false;
    }

private:
    bool process_events(Events* events);
public:
    virtual void process_midi_event(MidiEvent*) {}

    virtual bool parameter_can_be_automated(i32 parameter_id) const
    {
        (void)parameter_id;
        return true;
    }

    virtual char const* preset_name_from_id(i32 preset_id) const
    {
        (void)preset_id;
        return "noname";
    }

    virtual PinProperties input_properties(i32 input_id) const
    {
        (void)input_id;
        return {};
    }
    virtual PinProperties output_properties(i32 output_id) const
    {
        (void)output_id;
        return {};
    }

    [[nodiscard]] virtual bool notify_offline(AudioFile* files,
                                i32 files_size,
                                bool start)
    {
        (void)files;
        (void)files_size;
        (void)start;
        return false;
    }

    [[nodiscard]] virtual bool prepare_offline(OfflineTask* tasks,
                                 i32 tasks_size)
    {
        (void)tasks;
        (void)tasks_size;
        return false;
    }

    [[nodiscard]] virtual bool run_offline(OfflineTask* tasks, i32 tasks_size)
    {
        (void)tasks;
        (void)tasks_size;
        return false;
    }

    [[nodiscard]] virtual bool set_speaker_arrangement(SpeakerArrangement* value)
    {
        (void)value;
        return false;
    }

    [[nodiscard]] virtual bool process_variable_io(VariableIO* io)
    {
        (void)io;
        return false;
    }

    [[nodiscard]] virtual bool set_bypass(bool value)
    {
        (void)value;
        return false;
    }

private:
    iptr internal_vendor_specific(i32 index, iptr value,
                                      void* ptr, f32 opt)
    {
        if (index == 1349674323 && value == 1097155443)
            return set_editor_dpi(opt), true;
        return vendor_specific(index, value, ptr, opt);
    }
public:
    virtual iptr vendor_specific(i32 index, iptr value,
                                     void* ptr, f32 opt)
    {
        (void)index;
        (void)value;
        (void)ptr;
        (void)opt;
        return 0;
    }

    virtual CanDo can_do(char const*) const
    {
        return CanDo::No;
    }

    virtual i32 tail_size() const
    {
        return 0;
    }

    virtual MidiPresetName const* midi_preset_name(i32 preset_id) const
    {
        (void)preset_id;
        return nullptr;
    }

    [[nodiscard]] virtual bool set_total_samples_to_process(i32 value)
    {
        (void)value;
        return false;
    }

    [[nodiscard]] virtual bool set_pan_law(PanLaw type, f32 gain)
    {
        (void)type;
        (void)gain;
        return false;
    }

    [[nodiscard]] virtual bool set_process_precision(Precision value)
    {
        (void)value;
        return false;
    }

    virtual i32 midi_input_channels_size() const
    {
        return 0;
    }

    virtual i32 midi_output_channels_size() const
    {
        return 0;
    }

    virtual void set_parameter(i32 parameter_id, f32 value)
    {
        (void)parameter_id;
        (void)value;
    }

    virtual f32 parameter(i32 parameter_id) const
    {
        (void)parameter_id;
        return __builtin_nanf("");
    }

    virtual ParameterProperties parameter_properties(i32 parameter_id) const
    {
        (void)parameter_id;
        return {};
    }

    virtual Rectangle const* editor_rectangle() const
    {
        return nullptr;
    }

    virtual bool set_editor_rectangle(Rectangle const*)
    {
        return false;
    }

    [[nodiscard]] virtual bool set_knob_mode(KnobMode mode)
    {
        (void)mode;
        return false;
    }

    iptr dispatch_prosonus_extension(ProsonusPluginOpcode opcode, void* ptr, f32 opt);
    iptr dispatch(PluginOpcode opcode, i32 index, iptr value, void* ptr, f32 opt);
};

}

#pragma once
#include <Vst/AEffect.h>
#include <Vst/Vst.h>
#include <Ty/Base.h>
#include <Vst/KnobMode.h>
#include <Vst/Precision.h>
#include <Vst/Rectangle.h>
#include <Vst/ChunkType.h>
#include <Vst/CanDo.h>
#include <Vst/Host.h>
#include <Vst/Config.h>

namespace Vst {

struct AudioPlugin {
    AudioPlugin() = default;;
    virtual ~AudioPlugin() = default;

    virtual Config config() const = 0;

    // static AudioPlugin* create(Host host);
    // void destroy();

    virtual bool open_editor(void* window_handle) = 0;
    virtual void editor_loop() = 0;
    virtual void close_editor() = 0;
    virtual void set_editor_dpi(f32 value) = 0;

    [[nodiscard]] virtual bool resume() = 0;
    [[nodiscard]] virtual bool suspend() = 0;

    [[nodiscard]] virtual bool start_process() = 0;
    [[nodiscard]] virtual bool stop_process() = 0;

    virtual void process_f32(f32 const* const* inputs,
                             f32* const* outputs,
                             i32 samples) = 0;

    virtual void process_f64(f64 const* const* inputs,
                             f64* const* outputs,
                             i32 samples) = 0;

    [[nodiscard]] virtual bool set_current_preset(i32 id) = 0;
    virtual i32 current_preset() const = 0;
    [[nodiscard]] virtual bool set_preset_name(char const* new_name) = 0;
    virtual char const* preset_name() const = 0;

    virtual char const* parameter_label(i32 parameter_id) const = 0;
    virtual char const* parameter_display(i32 parameter_id) const = 0;
    virtual char const* parameter_name(i32 parameter_id) const = 0;

    [[nodiscard]] virtual bool set_sample_rate(f32 value) = 0;
    [[nodiscard]] virtual bool set_block_size(i32 value) = 0;

    // bool get_chunk(ChunkType type);
    [[nodiscard]] virtual bool set_chunk(ChunkType type, i32 id, void* value) = 0;

private:
    bool process_events(Events* events);
public:
    virtual void process_midi_event(MidiEvent*) = 0;

    virtual bool parameter_can_be_automated(i32 parameter_id) const = 0;

    // static bool string_to_parameter(i32 id, char const* name);

    virtual char const* preset_name_from_id(i32 preset_id) const = 0;

    virtual PinProperties input_properties(i32 input_id) const = 0;
    virtual PinProperties output_properties(i32 output_id) const = 0;

    [[nodiscard]] virtual bool notify_offline(AudioFile* files,
                                i32 files_size,
                                bool start) = 0;

    [[nodiscard]] virtual bool prepare_offline(OfflineTask* tasks,
                                 i32 tasks_size) = 0;

    [[nodiscard]] virtual bool run_offline(OfflineTask* tasks, i32 tasks_size) = 0;

    [[nodiscard]] virtual bool set_speaker_arrangement(SpeakerArrangement* value) = 0;

    [[nodiscard]] virtual bool process_variable_io(VariableIO* io) = 0;

    [[nodiscard]] virtual bool set_bypass(bool value) = 0;

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

    virtual CanDo can_do(char const* thing) const = 0;

    virtual i32 tail_size() const = 0;

    virtual MidiPresetName const* midi_preset_name(i32 preset_id) const = 0;

    [[nodiscard]] virtual bool set_total_samples_to_process(i32 value) = 0;

    [[nodiscard]] virtual bool set_pan_law(PanLaw type, f32 gain) = 0;

    [[nodiscard]] virtual bool set_process_precision(Precision value) = 0;

    virtual i32 midi_input_channels_size() const = 0;
    virtual i32 midi_output_channels_size() const = 0;

    virtual void set_parameter(i32 parameter_id, f32 value) = 0;

    virtual f32 parameter(i32 parameter_id) const = 0;

    virtual ParameterProperties parameter_properties(i32 parameter_id) const = 0;

#if 0
    virtual void set_imgui_context(ImGuiContext* context)
    {
        m_imgui_context = context;
    }
#endif

    virtual Rectangle const* editor_rectangle() const = 0;

    [[nodiscard]] virtual bool set_knob_mode(KnobMode mode) = 0;

    iptr dispatch(PluginOpcode opcode, i32 index, iptr value, void* ptr, f32 opt);

#if 0
    ImGuiContext* m_imgui_context { nullptr };
    SDL_Window* m_window { nullptr };
    SDL_Renderer* m_renderer { nullptr };
    SDL_Thread* m_gui_thread { nullptr };
    bool m_gui_thread_should_quit { false };
    VSTRectangle m_editor_rectangle {
        .y = 0,
        .x = 0,
        .height = 500,
        .width = 800
    };
#endif
};

}

#pragma once
#include <Vst/AEffect.h>
#include <Vst/AudioPlugin.h>
#include <Vst/CanDo.h>

struct BareMinimum final : Vst::AudioPlugin {
    BareMinimum() = default;
    ~BareMinimum() override = default;

    Vst::Config config() const override
    {
        return {
            .plugin_name = "BareMinimum",
            .author_name = "Junior Rantila",
            .product_name = "BareMinimum",
            .category = Vst::PluginCategory::Effect,
            .flags = (i32)Vst::PluginFlags::CanF32Replacing,
            .author_id = 1337,
            .plugin_version = 0,
            .number_of_inputs = 0,
            .number_of_outputs = 0,
            .number_of_presets = 0,
            .number_of_parameters = 0,
            .initial_delay = 0,
        };
    }

    bool open_editor(Vst::NativeHandle window_handle) override
    {
        (void)window_handle;
        return false;
    }

    void editor_loop() override { }

    void close_editor() override { }

    bool suspend() override
    {
        return false;
    }

    bool resume() override
    {
        return false;
    }

    void process_f32(f32 const* const* inputs, f32* const* outputs,
        i32 samples) override
    {
        (void)inputs;
        (void)outputs;
        (void)samples;
    }

    void process_f64(f64 const* const* inputs, f64* const* outputs,
        i32 samples) override
    {
        (void)inputs;
        (void)outputs;
        (void)samples;
    }

    bool set_current_preset(i32 id) override
    {
        (void)id;
        return false;
    }

    i32 current_preset() const override
    {
        return 0;
    }

    bool set_preset_name(char const* new_name) override
    {
        (void)new_name;
        return false;
    }

    char const* preset_name() const override
    {
        return nullptr;
    }

    char const* parameter_label(i32 parameter_id) const override
    {
        (void)parameter_id;
        return nullptr;
    }

    char const* parameter_display(i32 parameter_id) const override
    {
        (void)parameter_id;
        return nullptr;
    }

    char const* parameter_name(i32 parameter_id) const override
    {
        (void)parameter_id;
        return nullptr;
    }

    bool set_sample_rate(f32 value) override
    {
        (void)value;
        return false;
    }

    bool set_block_size(i32 value) override
    {
        (void)value;
        return false;
    }

    // bool get_chunk(ChunkType type);
    bool set_chunk(Vst::ChunkType type, i32 id,
        void* value) override
    {
        (void)type;
        (void)id;
        (void)value;
        return false;
    }

    void process_midi_event(Vst::MidiEvent* event) override
    {
        (void)event;
    }

    bool parameter_can_be_automated(i32 parameter_id) const override
    {
        (void)parameter_id;
        return false;
    }

    char const* preset_name_from_id(i32 id) const override
    {
        (void)id;
        return nullptr;
    }

    Vst::PinProperties input_properties(i32 id) const override
    {
        (void)id;
        return Vst::PinProperties {};
    }

    Vst::PinProperties output_properties(i32 id) const override
    {
        (void)id;
        return Vst::PinProperties {};
    }

    bool notify_offline(Vst::AudioFile* files, i32 files_size,
        bool start) override
    {
        (void)files;
        (void)files_size;
        (void)start;
        return false;
    }

    bool prepare_offline(Vst::OfflineTask* tasks,
        i32 tasks_size) override
    {
        (void)tasks;
        (void)tasks_size;
        return false;
    }

    bool run_offline(Vst::OfflineTask* tasks,
        i32 tasks_size) override
    {
        (void)tasks;
        (void)tasks_size;
        return false;
    }

    bool set_speaker_arrangement(Vst::SpeakerArrangement* value) override
    {
        (void)value;
        return false;
    }

    bool process_variable_io(Vst::VariableIO* io) override
    {
        (void)io;
        return false;
    }

    bool set_bypass(bool value) override
    {
        (void)value;
        return false;
    }

    Vst::CanDo can_do(char const* thing) const override
    {
        (void)thing;
        return Vst::CanDo::No;
    }

    i32 tail_size() const override { return 0; }

    Vst::ParameterProperties parameter_properties(i32 parameter_id) const override
    {
        (void)parameter_id;
        return Vst::ParameterProperties {};
    }

    Vst::MidiPresetName const* midi_preset_name(i32 id) const override
    {
        (void)id;
        return nullptr;
    }

    bool set_total_samples_to_process(i32 value) override
    {
        (void)value;
        return false;
    }

    bool start_process() override
    {
        return false;
    }

    bool stop_process() override
    {
        return false;
    }

    bool set_pan_law(Vst::PanLaw type, f32 gain) override
    {
        (void)type;
        (void)gain;
        return false;
    }

    bool set_process_precision(Vst::Precision value) override
    {
        (void)value;
        return false;
    }

    i32 midi_input_channels_size() const override
    {
        return 0;
    }

    i32 midi_output_channels_size() const override
    {
        return 0;
    }

    void set_parameter(i32 id, f32 value) override
    {
        (void)id;
        (void)value;
    }

    f32 parameter(i32 id) const override
    {
        (void)id;
        return 0;
    }

    Vst::Rectangle const* editor_rectangle() const override
    {
        return nullptr;
    }

    bool set_knob_mode(Vst::KnobMode mode) override
    {
        (void)mode;
        return false;
    }
};

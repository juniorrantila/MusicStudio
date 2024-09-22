#pragma once
#include <Vst/AEffect.h>
#include <Vst/ChunkType.h>
#include <Vst/Host.h>
#include <Vst/KnobMode.h>
#include <Vst/Rectangle.h>
#include <Vst/Vst.h>
#include <Ty/Base.h>
#include <Vst/Precision.h>
#include <Vst/CanDo.h>
#include <Midi/Note.h>
#include <Vst/AudioPlugin.h>

struct AudioPlugin final : Vst::AudioPlugin {
    enum class Parameter : u32;
    enum class Output : u32 {
        Left,
        Right,
        __Size
    };

    AudioPlugin() {}
    ~AudioPlugin() override {}

    Vst::Config config() const override
    {
        return {
            .plugin_name = "AudioPlugin",
            .author_name = "Junior Rantila",
            .product_name = "AudioPlugin",
            .category = Vst::PluginCategory::Synth,
            .flags = (i32)Vst::PluginFlags::CanF32Replacing
                   | (i32)Vst::PluginFlags::HasEditor
                   | (i32)Vst::PluginFlags::IsSynth
                   | (i32)Vst::PluginFlags::IsSilentWhenStopped
                   ,
            .author_id = 1337,
            .plugin_version = 0,
            .number_of_inputs = 0,
            .number_of_outputs = (u32)Output::__Size,
            .number_of_presets = 0,
            .number_of_parameters = 1,
            .initial_delay = 0,
        };
    }

    [[nodiscard]] bool open_editor(Vst::NativeHandle handle) override;
    void close_editor() override;

    void process_f32(f32 const* const* inputs,
                     f32* const* outputs,
                     i32 samples) override;

    [[nodiscard]] bool set_sample_rate(f32 value) override;
    [[nodiscard]] bool set_block_size(i32 value) override;

    Vst::Rectangle const* editor_rectangle() const override
    {
        return &m_editor_rectangle;
    }

    void set_host(Vst::Host host)
    {
        m_host = host;
    }

    c_string parameter_name(i32) const override
    {
        return "Frequency";
    }

    void set_parameter(i32, f32 value) override
    {
        m_freq = value * 1000.0f;
    }

    f32 parameter(i32) const override
    {
        return m_freq / 1000.0f;
    }

private:
    Vst::Host m_host { nullptr, nullptr };
    Vst::NativeHandle m_window_handle { nullptr };
    Vst::NativeHandle m_view { nullptr };
    Vst::Rectangle m_editor_rectangle {
        .y = 0,
        .x = 0,
        .height = 320,
        .width = 440
    };

    f32 m_freq { 440.0f }; 
    f32 m_sample_rate { 44000.0f };
    i32 m_block_size { 0 };
    f32 m_seconds_offset { 0.0f };
};

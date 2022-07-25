#pragma once
#include <Vst/AEffect.h>
#include <Vst/ChunkType.h>
#include <Vst/Host.h>
#include <Vst/KnobMode.h>
#include <Vst/Rectangle.h>
#include <Vst/Vst.h>
#include <JR/Types.h>
#include <Vst/Precision.h>
#include <Vst/CanDo.h>
#include <cmath>
#include <type_traits>
#include <imgui/imgui.h>
#include <Midi/Note.h>
#include <Vst/AudioPlugin.h>

struct AudioFile;
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Thread;

struct AudioPlugin final : Vst::AudioPlugin {
    enum class Parameter : u32;
    enum class Input : u32 {
        Left,
        Right,
        __Size
    };
    enum class Output : u32 {
        Left,
        Right,
        __Size
    };

    AudioPlugin();
    ~AudioPlugin() override;

    // static AudioPlugin* create(Host host);
    // void destroy() override;
    
    Vst::Config config() const override
    {
        return {
            .plugin_name = "Hello, World!",
            .author_name = "Junior Rantila",
            .product_name = "<product name>",
            .category = Vst::PluginCategory::Synth,
            .flags = (i32)Vst::PluginFlags::CanF32Replacing
                   | (i32)Vst::PluginFlags::CanF64Replacing
                   | (i32)Vst::PluginFlags::HasEditor
                   | (i32)Vst::PluginFlags::IsSynth
                   ,
            .author_id = 1337,
            .plugin_version = 0,
            // .number_of_inputs = (u32)Input::__Size,
            .number_of_inputs = 0,
            .number_of_outputs = (u32)Output::__Size,
            .number_of_presets = 1,
            .number_of_parameters = (u32)Parameter::__Size,
            .initial_delay = 0,
        };
    }

    [[nodiscard]] bool open_editor(void* window_handle) override;
    void editor_loop() override;
    void close_editor() override;
    void set_editor_dpi(f32 value) override { m_editor_dpi = value; }

    [[nodiscard]] bool suspend() override;
    [[nodiscard]] bool resume() override;

    template<typename T>
        requires(std::is_floating_point<T>())
    void process_audio(T const* const*, T* const*, i32);

    void process_f32(f32 const* const* inputs,
                     f32* const* outputs,
                     i32 samples) override;

    void process_f64(f64 const* const* inputs,
                     f64* const* outputs,
                     i32 samples) override;

    [[nodiscard]] bool set_current_preset(i32 id) override;
    i32 current_preset() const override;
    [[nodiscard]] bool set_preset_name(char const* new_name) override;
    char const* preset_name() const override;

    char const* parameter_label(Parameter) const;
    char const* parameter_display(Parameter) const;
    char const* parameter_name(Parameter) const;

    char const* parameter_label(i32 parameter_id) const override
    {
        if (parameter_id < 0 || parameter_id > (i32)Parameter::__Size)
            return nullptr;
        return parameter_label((Parameter)parameter_id);
    }

    char const* parameter_display(i32 parameter_id) const override
    {
        if (parameter_id < 0 || parameter_id > (i32)Parameter::__Size)
            return nullptr;
        return parameter_display((Parameter)parameter_id);
    }

    char const* parameter_name(i32 parameter_id) const override
    {
        if (parameter_id < 0 || parameter_id > (i32)Parameter::__Size)
            return nullptr;
        return parameter_name((Parameter)parameter_id);
    }

    [[nodiscard]] bool set_sample_rate(f32 value) override;
    [[nodiscard]] bool set_block_size(i32 value) override;

    // bool get_chunk(ChunkType type);
    [[nodiscard]] bool set_chunk(Vst::ChunkType type, i32 id, void* value) override;

    void process_midi_event(Vst::MidiEvent*) override;

    bool parameter_can_be_automated(Parameter parameter) const;
    bool parameter_can_be_automated(i32 parameter_id) const override
    {
        if (parameter_id < 0 || parameter_id >= (i32)Parameter::__Size)
            return false;
        return parameter_can_be_automated((Parameter)parameter_id);
    }

    // static bool string_to_parameter(i32 id, char const* name);

    char const* preset_name_from_id(i32 id) const override;

    Vst::PinProperties input_properties(Input) const;
    Vst::PinProperties output_properties(Output) const;

    Vst::PinProperties input_properties(i32 id) const override
    {
        // FIXME: if (id < 0 || id >= Input::__Size)
        return input_properties((Input)id);
    }

    Vst::PinProperties output_properties(i32 id) const override
    {
        // FIXME: if (id < 0 || id >= Output::__Size)
        return output_properties((Output)id);
    }

    [[nodiscard]] bool notify_offline(Vst::AudioFile* files,
                        i32 files_size,
                        bool start) override;

    [[nodiscard]] bool prepare_offline(Vst::OfflineTask* tasks,
                         i32 tasks_size) override;

    [[nodiscard]] bool run_offline(Vst::OfflineTask* tasks,
                     i32 tasks_size) override;


    [[nodiscard]] bool set_speaker_arrangement(Vst::SpeakerArrangement* value) override;

    [[nodiscard]] bool process_variable_io(Vst::VariableIO* io) override;

    [[nodiscard]] bool set_bypass(bool value) override;

    Vst::CanDo can_do(char const* thing) const override;

    i32 tail_size() const override;

    Vst::ParameterProperties parameter_properties(Parameter) const;
    Vst::ParameterProperties parameter_properties(i32 parameter_id) const override
    {
        // FIXME: if (parameter_id < 0 || parameter_id >= Parameter::__Size)
        return parameter_properties((Parameter)parameter_id);
    }

    Vst::MidiPresetName const* midi_preset_name(i32 id) const override;

    [[nodiscard]] bool set_total_samples_to_process(i32 value) override;

    [[nodiscard]] bool start_process() override;
    [[nodiscard]] bool stop_process() override;

    [[nodiscard]] bool set_pan_law(Vst::PanLaw type, f32 gain) override;

    [[nodiscard]] bool set_process_precision(Vst::Precision value) override;

    i32 midi_input_channels_size() const override;
    i32 midi_output_channels_size() const override;

    void set_parameter(Parameter id, f32 value);
    void set_parameter(i32 id, f32 value) override
    {
        if (id < 0 || id >= (i32)Parameter::__Size)
            return;
        set_parameter((Parameter)id, value);
    }

    f32 parameter(Parameter id) const;
    f32 parameter(i32 id) const override
    {
        if (id < 0 || id >= (i32)Parameter::__Size)
            return NAN;
        return parameter((Parameter)id);
    }

#if 0
    void set_imgui_context(ImGuiContext* context)
    {
        m_imgui_context = context;
    }
#endif

    Vst::Rectangle const* editor_rectangle() const override
    {
        return &m_editor_rectangle;
    }

    [[nodiscard]] bool set_knob_mode(Vst::KnobMode mode) override;

    void set_host(Vst::Host host)
    {
        m_host = host;
    }

private:
    Vst::Host m_host { nullptr, nullptr };
    ImGuiContext* m_imgui_context { nullptr };
    void* m_window_handle { nullptr };
    SDL_Window* m_sdl_window { nullptr };
    SDL_Renderer* m_renderer { nullptr };
    SDL_Thread* m_gui_thread { nullptr };
    bool m_gui_thread_should_quit { false };
    Vst::Rectangle m_editor_rectangle {
        .y = 0,
        .x = 0,
        .height = 320,
        .width = 440
    };
    f32 m_editor_dpi { 1.0 };

    u32 m_sample_rate { 1 };
    i32 m_block_size { 0 };
    u32 m_current_block { 0 };

    void increment_block()
    {
        m_current_block = (m_current_block + m_block_size) % m_sample_rate;
    }

public:
    enum class Parameter : u32 {
        Sine,
        Triangle,
        Square,
        __Size,
    };

    static constexpr char const* s_parameter_names[(u32)Parameter::__Size] {
#define PARAMETER_NAME(__name) [(u32)Parameter::__name] = #__name
        PARAMETER_NAME(Sine),
        PARAMETER_NAME(Triangle),
        PARAMETER_NAME(Square),
#undef PARAMETER_NAME
    };
private:

    f32 m_parameters[(u32)Parameter::__Size] {
        [(u32)Parameter::Sine] = 1.0,
        [(u32)Parameter::Triangle]  = 0.0,
        [(u32)Parameter::Square]  = 0.0,
    };

    // clang-format off
    Vst::ParameterProperties m_parameters_properties[(u32)Parameter::__Size] {
        [(u32)Parameter::Sine] = Vst::ParameterProperties {
            .step_float = 1.0,
            .small_step_float = 0.1,
            .large_step_float = 10.0,
            .label { "Sine" },
            .flags = (u32)Vst::ParameterFlags::UsesFloatStep
                   | (u32)Vst::ParameterFlags::UsesIntegerMinMax
                   | (u32)Vst::ParameterFlags::CanRampUpOrDown
                   ,
            .min_integer = 0,
            .max_integer = 100,
            .step_integer = 1,
            .large_step_integer = 10,
            .short_label { "Sine" },

            .display_index = 0,

            .category = 0,
            .parameters_in_category = 0,
            .category_label = { 0 }
        },
        [(u32)Parameter::Triangle]  = Vst::ParameterProperties {
            .step_float = 1.0,
            .small_step_float = 0.1,
            .large_step_float = 10.0,
            .label { "Triangle" },
            .flags = (u32)Vst::ParameterFlags::UsesFloatStep
                   | (u32)Vst::ParameterFlags::UsesIntegerMinMax
                   | (u32)Vst::ParameterFlags::CanRampUpOrDown
                   ,
            .min_integer = 0,
            .max_integer = 100,
            .step_integer = 1,
            .large_step_integer = 9,
            .short_label { "Triangl" },

            .display_index = 0,

            .category = 0,
            .parameters_in_category = 0,
            .category_label = { 0 }

        },
        [(u32)Parameter::Square] = Vst::ParameterProperties {
            .step_float = 1,
            .small_step_float = 0.1,
            .large_step_float = 10.0,
            .label { "Square" },
            .flags = (u32)Vst::ParameterFlags::UsesFloatStep
                   | (u32)Vst::ParameterFlags::UsesIntegerMinMax
                   | (u32)Vst::ParameterFlags::CanRampUpOrDown
                   ,
            .min_integer = 0,
            .max_integer = 100,
            .step_integer = 1,
            .large_step_integer = 9,
            .short_label { "Square" },

            .display_index = 0,

            .category = 0,
            .parameters_in_category = 0,
            .category_label = { 0 }

        },
    };
    // clang-format on

    Vst::ParameterProperties& m_sine_properties {
        m_parameters_properties[(u32)Parameter::Sine]
    };

    Vst::ParameterProperties& m_triangle_properties {
        m_parameters_properties[(u32)Parameter::Triangle]
    };

    Vst::ParameterProperties& m_square_properties {
        m_parameters_properties[(u32)Parameter::Square]
    };
    
    // clang-format off
    Vst::PinProperties m_input_properties[(u32)Input::__Size] {
        [(u32)Input::Left] = Vst::PinProperties {
            .label = "Left input",
            .flags = (u32)Vst::PinPropertiesFlags::IsStereo
                   | (u32)Vst::PinPropertiesFlags::IsActive
                   | (u32)Vst::PinPropertiesFlags::UseSpeaker
                   ,
            .arrangement_type = 0,
            .short_label = "Li",
        },
        [(u32)Input::Right] = Vst::PinProperties {
            .label = "Right input",
            .flags = (u32)Vst::PinPropertiesFlags::IsStereo
                   | (u32)Vst::PinPropertiesFlags::IsActive
                   | (u32)Vst::PinPropertiesFlags::UseSpeaker
                   ,
            .arrangement_type = 0,
            .short_label = "Ri",
        }
    };
    Vst::PinProperties m_output_properties[(u32)Output::__Size] {
        [(u32)Output::Left] = Vst::PinProperties {
            .label = "Left output",
            .flags = (u32)Vst::PinPropertiesFlags::IsStereo
                   | (u32)Vst::PinPropertiesFlags::IsActive
                   | (u32)Vst::PinPropertiesFlags::UseSpeaker
                   ,
            .arrangement_type = 0,
            .short_label = "Lo",
        },
        [(u32)Output::Right] = Vst::PinProperties {
            .label = "Right output",
            .flags = (u32)Vst::PinPropertiesFlags::IsStereo
                   | (u32)Vst::PinPropertiesFlags::IsActive
                   | (u32)Vst::PinPropertiesFlags::UseSpeaker
                   ,
            .arrangement_type = 0,
            .short_label = "Ro",
        }
    };
    // clang-format on

    constexpr f32& parameter_ref(Parameter value)
    {
        return m_parameters[(i32)value];
    }
    Parameter m_current_parameter { Parameter::Sine };

    f32& m_sine { parameter_ref(Parameter::Sine) };
    f32& m_triangle { parameter_ref(Parameter::Triangle) };
    f32& m_square { parameter_ref(Parameter::Square) };

    constexpr f32 sine_nature() const { return m_parameters[(i32)Parameter::Sine]; }
    constexpr f32 triangle_nature() const { return m_parameters[(i32)Parameter::Triangle]; }
    constexpr f32 square_nature() const { return m_parameters[(i32)Parameter::Square]; }

    Vst::PanLaw m_pan_law { Vst::PanLaw::Linar };
    f32 m_pan_law_gain { 0.0 };

    f32 m_playing_frequencies[(u8)Midi::Note::__Size] = {};
    u8 m_playing_frequencies_size { 0 };

    bool m_is_note_playing[(u8)Midi::Note::__Size] = {};
    constexpr bool is_note_playing(i8 note_id) const
    {
        return m_is_note_playing[static_cast<u8>(note_id)];
    }

    constexpr void update_playing_frequencies()
    {
        m_playing_frequencies_size = 0;
        for (u8 note = 0; note < (u8)Midi::Note::__Size; note++) {
            if (m_is_note_playing[note]) [[unlikely]] {
                m_playing_frequencies[m_playing_frequencies_size++] = 
                    Midi::note_frequency((Midi::Note)note);
            }
        }
    }

    constexpr bool notes_are_playing() const
    {
        return m_playing_frequencies_size != 0;
    }


    constexpr void play_note(Midi::Note note)
    {
        m_is_note_playing[(u8)note] = true;
    }

    constexpr void mute_note(Midi::Note note)
    {
        m_is_note_playing[(u8)note] = false;
    }

    constexpr void mute_all_notes()
    {
        for (i8 note = 0; note < (i8)Midi::Note::__Size; note++)
            m_is_note_playing[(u8)note] = false;
    }

    struct PlayingNoteFrequenciesIterator {
        struct Iterator {
            bool const* is_note_playing;
            u8 i;

            constexpr f32 operator * () const
            {
                return Midi::note_frequency((Midi::Note)i);
            }
            constexpr bool operator == (Iterator const& other) const
            {
                return other.i == i;
            }
            constexpr void operator ++ ()
            {
                i++;
                while(!is_note_playing[i])
                    i++;
            }
        };

        constexpr Iterator begin() const
        {
            return {
                .is_note_playing = is_note_playing,
                .i = 0,
            };
        }

        constexpr Iterator end() const {
            return {
                .i = (u8)Midi::Note::__Size,
            };
        }
        
        bool const* is_note_playing;
    };
    constexpr PlayingNoteFrequenciesIterator playing_notes_frequencies()
    {
        return {
            .is_note_playing = m_is_note_playing
        };
    }

    // Precision m_process_pressicion { Precision::F32 };
};

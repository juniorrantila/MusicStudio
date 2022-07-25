#include "AudioPlugin.h"

#include <Fonts/OxaniumLight.h>
#include <GUI/Theme.h>
#include <Midi/Packet.h>
#include <SDL.h>
#include <SDL_render.h>
#include <Vst/AEffect.h>
#include <Vst/CanDo.h>
#include <Vst/Vst.h>

#include <cmath>
#include <imgui-knobs/imgui-knobs.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_sdlrenderer.h>
#include <imgui_internal.h>
#include <stdio.h>
#include <time.h>
#include <type_traits>

#include <JR/Defer.h>
#include <JR/Log.h>
#include <mutex>

thread_local ImGuiContext* g_imgui_context { nullptr };
std::mutex g_imgui_context_lock;

AudioPlugin::AudioPlugin()
{
    LOG("SESAME OPEN!");
    SDL_SetHint(SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT, "true");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "%s\n", SDL_GetError());
        fflush(nullptr);
        __builtin_abort();
    }
    IMGUI_CHECKVERSION();
}

AudioPlugin::~AudioPlugin()
{
    SDL_Quit();
}

#if 0
AudioPlugin* AudioPlugin::create(Host host)
{
}
#endif

#if 0
void AudioPlugin::destroy()
{
    LOG("we'll get them next time boys");
    delete this;
}
#endif

bool AudioPlugin::open_editor(void* window_handle)
{
    LOG("open editor %p", window_handle);
    m_window_handle = window_handle;
    m_gui_thread = SDL_CreateThread([](void* argument) -> int {
        auto& self = *((AudioPlugin*)argument);
        self.m_sdl_window = SDL_CreateWindowFrom(self.m_window_handle);
        if (!self.m_sdl_window) {
            // FIXME: Error
            return false;
        }

        self.m_renderer = SDL_CreateRenderer(self.m_sdl_window, -1,
            SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        if (!self.m_renderer) {
            SDL_DestroyWindow(self.m_sdl_window);
            // FIXME: Error
            return false;
        }

        self.m_imgui_context = ImGui::CreateContext();
        ImGui_ImplSDL2_InitForSDLRenderer(self.m_imgui_context, self.m_sdl_window, self.m_renderer);
        ImGui_ImplSDLRenderer_Init(self.m_imgui_context, self.m_renderer);
        Defer destroy = [&] {
            ImGui_ImplSDLRenderer_Shutdown(self.m_imgui_context);
            ImGui_ImplSDL2_Shutdown(self.m_imgui_context);
            ImGui::DestroyContext(self.m_imgui_context);
            self.m_imgui_context = nullptr;

            SDL_DestroyRenderer(self.m_renderer);
            self.m_renderer = nullptr;
            SDL_DestroyWindow(self.m_sdl_window);
            self.m_sdl_window = nullptr;
        };

        auto& context = *self.m_imgui_context;
        context.Style.ScaleAllSizes(3 * self.m_editor_dpi);
        context.IO.Fonts->AddFontFromMemoryCompressedBase85TTF(Fonts::oxanium_light, 24);
        GUI::Theme::apply_on(context.Style);

        self.editor_loop();

        return true;
    }, "GUI", this);

    return true;
}

void AudioPlugin::editor_loop()
{
    auto plugin_name = config().plugin_name;
    auto plugin_author = config().author_name;

    while (!m_gui_thread_should_quit) {
        {
            std::lock_guard guard { g_imgui_context_lock };
            ImGui::SetCurrentContext(m_imgui_context);
            SDL_Event event;
            while (SDL_PollEvent(&event))
                ImGui_ImplSDL2_ProcessEvent(&event); // Needs lock.
        }

        if (g_imgui_context_lock.try_lock()) {
            ImGui::SetCurrentContext(m_imgui_context);

            g_imgui_context = m_imgui_context;
            Defer render = [this] {
                ImGui::Render(); // Definitely needs lock.
                SDL_SetRenderDrawColor(m_renderer, 123, 32, 42, 255);
                SDL_RenderClear(m_renderer);
                auto draw_data = ImGui::GetDrawData(); // Needs lock.
                ImGui_ImplSDLRenderer_RenderDrawData(draw_data); // Needs lock.
                SDL_RenderPresent(m_renderer);
                g_imgui_context_lock.unlock();
            };

            ImGui_ImplSDLRenderer_NewFrame();   // Needs lock.
            ImGui_ImplSDL2_NewFrame();          // Needs lock.
            ImGui::NewFrame();                  // Needs lock.
            auto width  = m_editor_rectangle.width  + 2 * m_editor_dpi;
            auto height = m_editor_rectangle.height + 2 * m_editor_dpi;
            ImGui::SetNextWindowSize({ width, height }); // Needs lock.
            auto one = 1 * m_editor_dpi;
            ImGui::SetNextWindowPos({ -one, -one }); // Needs lock.
            {
                // clang-format off
                ImGui::Begin(config().plugin_name, nullptr,
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoDecoration); // Needs lock.
                // clang-format on
                Defer end = [&] {
                    ImGui::End(); // Needs lock
                };

                {
                    ImGui::BeginGroup();
                    Defer end_group = [] {
                        ImGui::EndGroup();
                    };
                    ImGui::Text("Name: %s", plugin_name);
                    ImGui::Text("Author: %s", plugin_author);
                }

                ImGui::Separator();

                {
                    ImGui::BeginGroup();
                    Defer end_group = [] {
                        ImGui::EndGroup();
                    };
                    f32 sine_value = m_sine * 100.0;
                    ImGuiKnobs::Knob(m_sine_properties.label, &sine_value,
                        m_sine_properties.min_integer,
                        m_sine_properties.max_integer,
                        m_sine_properties.step_float,
                        "%.f%%", ImGuiKnobVariant_Wiper);
                    m_sine = sine_value / 100.0;

                    ImGui::SameLine();

                    f32 triangle_value = m_triangle * 100.0;
                    ImGuiKnobs::Knob(m_triangle_properties.label, &triangle_value,
                        m_triangle_properties.min_integer,
                        m_triangle_properties.max_integer,
                        m_triangle_properties.step_float,
                        "%.f%%", ImGuiKnobVariant_Wiper);
                    m_triangle = triangle_value / 100.0;

                    ImGui::SameLine();

                    f32 square_value = m_square * 100.0;
                    ImGuiKnobs::Knob(m_square_properties.label, &square_value,
                        m_square_properties.min_integer,
                        m_square_properties.max_integer,
                        m_square_properties.step_float,
                        "%.f%%", ImGuiKnobVariant_Wiper);
                    m_square = square_value / 100.0;
                }
            }
        }
    }
}

void AudioPlugin::close_editor()
{
    LOG("it's getting cold");

    m_gui_thread_should_quit = true;
    SDL_WaitThread(m_gui_thread, nullptr);
    // SDL_KillThread(s_gui_thread);
}

bool AudioPlugin::suspend()
{
    LOG("am I suspended? :(");
    // mute_all_notes();
    for (i8 note = 0; note < (i8)Midi::Note::__Size; note++)
        m_is_note_playing[(u8)note] = false;
    return true;
}

bool AudioPlugin::resume()
{
    LOG("I shall resume my duties!");
    // mute_all_notes();
    for (i8 note = 0; note < (i8)Midi::Note::__Size; note++)
        m_is_note_playing[(u8)note] = false;
    return true;
}

template<typename T>
    requires(std::is_floating_point<T>())
void AudioPlugin::process_audio(T const* const* inputs,
    T* const* outputs,
    i32 samples_in_block)
{
    Defer call_increment_block = [&] {
        increment_block();
    };

    auto in_left = inputs[0];
    auto in_right = inputs[1];
    (void)in_left;
    (void)in_right;

    auto out_left = outputs[0];
    auto out_right = outputs[1];

    // update_playing_frequencies();
#if 0
    if (!notes_are_playing()) {
        for (i32 i = 0; i < samples; i++) {
            out_left[i] = 0;
            out_right[i] = 0;
        }
        return;
    }
#endif

    // auto time = m_host.time();

    // T phase_increment = 2.0 * M_PI * 440.0 / m_sample_rate;
    for (u16 i = 0; i < samples_in_block; i++) {
#if 0
        auto pos = m_current_block + i;
        out_left[i] = i[out_right] = cos(phase_increment * pos) * 0.5;
#endif
        out_left[i] = out_right[i] = 0.0;
    }
    
#if 1
    auto fraction_sine = sine_nature();
    // auto fraction_triangle = triangle_nature();
    // auto fraction_square = square_nature();
    for (u8 i = 0; i < (u8)Midi::Note::__Size; i++) {
        if (!m_is_note_playing[i])
            continue;
        auto frequency = Midi::note_frequency((Midi::Note)i);
        T phase_increment = 2.0 * M_PI * frequency / m_sample_rate;
        // u32 samples = m_sample_rate;
        for (i32 i = 0; i < samples_in_block; i++) {
            auto pos = m_current_block + i;
            auto sinewave = cos(phase_increment * pos) * 0.5;
            // auto triangle = asin(sinewave);
            // auto square = sinewave > 0 ? 1 : -1;
            out_left[i] = out_right[i] += sinewave * fraction_sine;
            // out_left[i] = out_right[i] += triangle * fraction_triangle;
            // out_left[i] = out_right[i] += square * fraction_square;
        }
    }
#else
    for (auto frequency : playing_notes_frequencies()) {
        T phase_increment = 2.0 * M_PI * frequency / m_sample_rate;
        for (i32 i = 0; i < samples; i++) {
            auto sinewave = cos(phase_increment * (sample_start + i));
            auto triangle = asin(sinewave);
            auto square = triangle > 0.5 ? 1 : 0;
            out_right[i] = out_left[i] += sinewave * m_pan;
            out_right[i] = out_left[i] += triangle * (1 - m_pan);
            out_right[i] = out_left[i] += square * (m_gain);
            // out_right[i] = out_left[i] *= m_gain;
        }
    }
#endif

#if 0

    for (i32 sample = 0; sample < samples; sample++) {
        out_left[sample] = in_left[sample] * gain();
        out_right[sample] = in_right[sample] * gain();

        if (m_pan_law == PanLaw::Linar) {
            out_left[sample] *= 1.0 - ((1 + pan()) / 2.0);
            out_right[sample] *= ((1 + pan()) / 2.0);
        } else {
            auto time = ((T)sample) * 440.0 / m_sample_rate;
            auto value = 0;
            if (fmod(time, 440.0) < 220.0)
                value = sin(time * M_PI * 2.0);
            if (fmod(time, 880.0) < 440.0)
                value *= 0.125;
            out_right[sample] = value;
            out_left[sample] = value;
        }
    }

#endif
}

void AudioPlugin::process_f32(f32 const* const* inputs,
    f32* const* outputs,
    i32 samples)
{
    process_audio(inputs, outputs, samples);
}

void AudioPlugin::process_f64(f64 const* const* inputs,
    f64* const* outputs,
    i32 samples)
{
    process_audio(inputs, outputs, samples);
}

bool AudioPlugin::set_current_preset(i32 id)
{
    (void)id;
    return true;
}

i32 AudioPlugin::current_preset() const
{
    auto preset_id = 0;
    return preset_id;
}

bool AudioPlugin::set_preset_name(char const* new_name)
{
    (void)new_name;
    LOG("new preset name: %s", new_name);
    return true;
}

char const* AudioPlugin::preset_name() const
{
    auto name = "noname";
    LOG("preset name: %s", name);
    return name;
}

char const* AudioPlugin::parameter_label(Parameter id) const
{
    return s_parameter_names[(u32)id];
}

char const* AudioPlugin::parameter_display(Parameter id) const
{
    return s_parameter_names[(u32)id];
}

char const* AudioPlugin::parameter_name(Parameter id) const
{
    return s_parameter_names[(u32)id];
}

bool AudioPlugin::set_sample_rate(f32 value)
{
    LOG("new sample rate: %f", value);
    m_sample_rate = value;
    return true;
}

bool AudioPlugin::set_block_size(i32 value)
{
    LOG("new block size: %d", value);
    m_block_size = value;
    return true;
}

#if 0
f32 AudioPlugin::vu()
{
    return 0;
}
#endif

// bool get_chunk(ChunkType type);
bool AudioPlugin::set_chunk(Vst::ChunkType type, i32 id, void* value)
{
    (void)type;
    (void)id;
    (void)value;
    return false;
}

void AudioPlugin::process_midi_event(Vst::MidiEvent* event)
{
    constexpr auto log_midi = false;
    auto generic_packet = (Midi::Packet*)event->midi_data;
    switch (generic_packet->type) {
    case Midi::PacketType::NoteOff: {
        auto packet = generic_packet->as_note_off();
        LOG_IF(log_midi, "midi: [OFF %s VEL %.2x]",
            Midi::note_string(packet.note),
            packet.velocity);
        mute_note(packet.note);
        break;
    }

    case Midi::PacketType::NoteOn: {
        auto packet = generic_packet->as_note_on();
        LOG_IF(log_midi, "midi: [ON  %s VEL %.2x]",
            Midi::note_string(packet.note),
            packet.velocity);
        if (packet.velocity)
            play_note(packet.note);
        else
            mute_note(packet.note);
        break;
    }

    case Midi::PacketType::Aftertouch: {
        auto packet = generic_packet->as_aftertouch();
        (void)packet;
        LOG_IF(log_midi, "midi: [AFT %s %.2x]",
            Midi::note_string(packet.note),
            packet.touch);
        break;
    }

    case Midi::PacketType::ContinousControll: {
        auto packet = generic_packet->as_continous_controll();
        (void)packet;
        LOG_IF(log_midi, "midi: [CC  %.2x %.2x]",
            packet.controller,
            packet.controller_value);

        // FIXME: Check if this is actually what is happening.
        if (packet.controller == 0x7b && packet.controller_value == 0)
            mute_all_notes();
        break;
    }

    case Midi::PacketType::PatchChange: {
        auto packet = generic_packet->as_patch_change();
        (void)packet;
        LOG_IF(log_midi, "midi: [PC  %.2x]", packet.instrument);
        break;
    }

    case Midi::PacketType::ChannelPressure: {
        auto packet = generic_packet->as_channel_pressure();
        (void)packet;
        LOG_IF(log_midi, "midi: [CP  %.2x]", packet.pressure);
        break;
    }

    case Midi::PacketType::PitchBend: {
        auto packet = generic_packet->as_pitch_bend();
        (void)packet;
        LOG_IF(log_midi, "midi: [PB  %.2x %.2x]",
            packet.least_significant_byte,
            packet.most_significant_byte);
        break;
    }

    case Midi::PacketType::NonMusicalCommands: {
        auto packet = generic_packet->as_non_musical_commands();
        (void)packet;
        LOG_IF(log_midi, "midi: [NME %.2x %.2x]",
            packet.unknown0,
            packet.unknown1);
        break;
    }
    }
}

bool AudioPlugin::parameter_can_be_automated(Parameter) const
{
    return true;
}

// bool AudioPlugin::string_to_parameter(i32 id, char const* name)
// {
// }

char const* AudioPlugin::preset_name_from_id(i32 id) const
{
    (void)id;
    return "noname";
}

Vst::PinProperties AudioPlugin::input_properties(Input id) const
{
    return m_input_properties[(u32)id];
}

Vst::PinProperties AudioPlugin::output_properties(Output id) const
{
    return m_output_properties[(u32)id];
}

#if 0
Vst::PluginCategory AudioPlugin::category()
{
    return PluginCategory::Synth;
}
#endif

bool AudioPlugin::notify_offline(Vst::AudioFile* files, i32 files_size,
    bool start)
{
    (void)files;
    (void)files_size;
    (void)start;
    return false;
}

bool AudioPlugin::prepare_offline(Vst::OfflineTask* tasks,
    i32 tasks_size)
{
    (void)tasks;
    (void)tasks_size;
    return false;
}

bool AudioPlugin::run_offline(Vst::OfflineTask* tasks, i32 tasks_size)
{
    (void)tasks;
    (void)tasks_size;
    return false;
}

bool AudioPlugin::set_speaker_arrangement(
    Vst::SpeakerArrangement* value)
{
    (void)value;
    return false;
}

bool AudioPlugin::process_variable_io(Vst::VariableIO* io)
{
    (void)io;
    return false;
}

bool AudioPlugin::set_bypass(bool value)
{
    (void)value;
    return false;
}

Vst::CanDo AudioPlugin::can_do(char const* thing) const
{
    constexpr auto log_can_do = false;
    auto can = "no";
    (void)can;
    Defer log_message = [&] {
        LOG_IF(log_can_do, "can do %s? %s", thing, can);
    };
    if (strcmp(thing, Vst::CanDoStrings::RecieveVstMidiEvent) == 0)
        return can = "yes", Vst::CanDo::Yes;
    if (strcmp(thing, Vst::CanDoStrings::ViewDpiScaling) == 0)
        return can = "yes", Vst::CanDo::Yes;
    return Vst::CanDo::No;
}

i32 AudioPlugin::tail_size() const
{
    return -1;
}

Vst::ParameterProperties AudioPlugin::parameter_properties(Parameter id) const
{
    return m_parameters_properties[(u32)id];
}

Vst::MidiPresetName const* AudioPlugin::midi_preset_name(i32 id) const
{
    (void)id;
    return nullptr;
}

bool AudioPlugin::set_total_samples_to_process(i32 value)
{
    (void)value;
    return false;
}

bool AudioPlugin::start_process()
{
    return true;
}

bool AudioPlugin::stop_process()
{
    return true;
}

bool AudioPlugin::set_pan_law(Vst::PanLaw type, f32 gain)
{
    m_pan_law = type;
    m_pan_law_gain = gain;
    return true;
}

bool AudioPlugin::set_process_precision(Vst::Precision value)
{
    (void)value;
    return false;
}

i32 AudioPlugin::midi_input_channels_size() const
{
    return 1;
}

i32 AudioPlugin::midi_output_channels_size() const
{
    return 0;
}

void AudioPlugin::set_parameter(Parameter id, f32 value)
{
    parameter_ref(id) = value;
}

f32 AudioPlugin::parameter(Parameter id) const
{
    return m_parameters[(u32)id];
}

bool AudioPlugin::set_knob_mode(Vst::KnobMode mode)
{
    (void)mode;
    LOG("knob_mode = %s", knob_mode_string(mode));
    return true;
}

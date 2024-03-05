#pragma once
#include <Midi/Packet.h>
#include <Ty/Base.h>
#include <Ty/Defer.h>
#include <Ty/ErrorOr.h>
#include <Ty/SmallCapture.h>
#include <Ty/StringView.h>
#include <Vst/AEffect.h>
#include <Vst/Opcodes.h>
#include <Vst/Vst.h>

namespace MS {

union Version {
    constexpr Version(u8 major, u8 minor, u8 micro, u8 patch)
        : major(major)
        , minor(minor)
        , micro(micro)
        , patch(patch)
    {
    }
    struct {
        u8 major;
        u8 minor;
        u8 micro;
        u8 patch;
    };
    u32 full;
};

struct Host {
    Vst::Effect* effect;

    f32** input_buffers;
    f32** output_buffers;
    i32 number_of_inputs;
    i32 number_of_outputs;

    u32 sample_rate;
    u32 block_size;
    bool effect_is_initialized { false };
    Vst::TimeInfo time_info;

    SmallCapture<void(i32, i32)> on_resize;

    static ErrorOr<Host*> create(Vst::Effect* effect);
    void destroy() const;

    f32 const* const* input_samples(u32 number_of_samples);
    f32* const* output_samples(u32 number_of_samples);

    void update_time_info(u32 request_mask);

    bool has_midi_input_port() const { return false; }
    void handle_midi_input_events(u32 number_of_samples) const;
    void send_midi_packet(Midi::Packet) const;

    bool begin_parameter_edit(i32 parameter_id);
    bool automate_parameter(i32 parameter_id, f32 value);
    bool end_parameter_edit(i32 parameter_id);

    static u32 vst_version() { return 2400; }
    i32 current_shell_id() const;
    void idle();
    bool want_midi() const;
    bool process_events(Vst::Events* events);

    bool io_changed();

    bool resize_window(i32 width, i32 height);

    // u32 sample_rate();
    // u32 block_size();
    u32 input_latency();
    u32 output_latency();

    Vst::ProcessLevel current_process_level();
    Vst::AutomationState automation_state();

    bool offline_start(i32 new_audio_files_size, i32 audio_files_size, Vst::AudioFile* audio_files);
    bool offline_read(bool read_source, Vst::OfflineOption* value, Vst::OfflineTask* task);
    bool offline_write();
    void current_offline_pass();
    void current_offline_meta_pass();

    StringView author_name() { return "Junior Rantila"; }
    StringView host_name() { return "MusicStudio"; }
    Version vendor_version() { return { 0, 0, 0, 1 }; }

    iptr vendor_specific(i32 index, iptr value, void* ptr, f32 opt);

    bool can_do(char const* thing);

    Vst::HostLanguage language() const
    {
        return Vst::HostLanguage::English;
    }

#ifdef __apple__
    FSSpec directory();
#else
    char const* directory();
#endif
    void update_display();

    bool open_file_selector(Vst::FileSelect*);
    void close_file_selector(Vst::FileSelect*);

    static iptr dispatch(Vst::Effect* effect, Vst::HostOpcode opcode,
        i32 index, iptr value, void* ptr, f32 opt);
};

}

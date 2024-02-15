#include "Host.h"

#include <Ty/ScopeGuard.h>
#include <Ty/Base.h>
#include <Vst/AEffect.h>
#include <Vst/Opcodes.h>
#include <Vst/PluginData.h>
#include <Vst/Rectangle.h>
#include <Vst/Vst.h>
#include <stdio.h>

#include <sys/resource.h> // getpriority

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define LOG_IF(cond, fmt, ...) do { if (cond) { fprintf(stderr, "VST Host: " fmt "\n", ## __VA_ARGS__); } } while(0)
#define LOG(fmt, ...) fprintf(stderr, "VST Host: " fmt "\n", ## __VA_ARGS__)

namespace MS {

ErrorOr<Host*> Host::create(Vst::Effect* effect)
{
    auto number_of_inputs = effect->number_of_inputs;
    auto number_of_outputs = effect->number_of_outputs;
    u32 sample_rate = 44000;
    auto host = new Host {
        .effect = effect,
        .input_buffers = new f32*[number_of_inputs],
        .output_buffers = new f32*[number_of_outputs],
        .number_of_inputs = effect->number_of_inputs,
        .number_of_outputs = effect->number_of_outputs,
        .sample_rate = sample_rate
    };

    return host;
}

void Host::handle_midi_input_events(u32) const
{
}

void Host::destroy() const
{
}

f32 const* const* Host::input_samples(u32)
{
    return nullptr;
}

f32* const* Host::output_samples(u32)
{
    return nullptr;
}

void Host::send_midi_packet(Midi::Packet packet) const
{
    auto* events = (Vst::Events*)malloc(sizeof(Vst::Events)+sizeof(Vst::Event*)*4);
    events->number_of_events = 1;
    Vst::MidiEvent event = {
        .type = Vst::EventType::Midi,
        .struct_size_excluding_type_field_and_this = sizeof(Vst::MidiEvent),
        .delta_frames = 0,
        .flags = (i32)Vst::MidiEventFlags::IsRealtime,
        .note_length_in_samples = 0,
        .note_offset_in_samples = 0,
        .midi_data = {
            (u8)packet.type,
            packet.data[0],
            packet.data[1],
            packet.data[2]
        },
        .detune_cents = 0,
        .note_off_velocity = 127,
    };
    events->events[0] = (Vst::Event*)&event;
    (void)effect->process_events(events);
    // free(events);
}

iptr Host::dispatch(Vst::Effect* effect, Vst::HostOpcode opcode,
    i32 index, iptr value, void* ptr, f32 opt)
{
    constexpr auto should_log_opcodes = true;

    auto host = effect ? (Host*)effect->host_data : nullptr;
    if (opcode != Vst::HostOpcode::GetTime) {
        auto const* name = host_opcode_string(opcode);
        auto parens = !index && !value && !ptr && opt == 0 ? "()" : "";
        if (name)
            LOG_IF(should_log_opcodes, "%s%s", name, parens);
        else
            LOG_IF(should_log_opcodes, "%d%s", opcode, parens);
        if (parens[0] == '\0') {
            if (index)
                LOG_IF(should_log_opcodes, "    index: %d", index);
            if (value)
                LOG_IF(should_log_opcodes, "    value: %ld", value);
            if (ptr)
                LOG_IF(should_log_opcodes, "    ptr:   0x%p", ptr);
            if (opt)
                LOG_IF(should_log_opcodes, "    opt:   %f", opt);
        }
    }
    using enum Vst::HostOpcode;
    switch (opcode) {
    case Automate:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->automate_parameter(index, opt);

    case VstVersion:
        return Host::vst_version();

    case CurrentId:
        return host->current_shell_id();

    case Idle:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        host->idle();
        return 1;

    case _PinConnected:
        return 0;

    case _WantMidi:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->want_midi();

    case GetTime:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        host->update_time_info(index);
        return (iptr)&host->time_info;

    case ProcessEvents:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->process_events((Vst::Events*)ptr);

    case _SetTime:
        return 0;

    case _TempoAt:
        return 0;

    case _GetNumAutomatableParameters:
        return 0;

    case _GetParameterQuantization:
        return 0;

    case IOChanged:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->io_changed();

    case _NeedIdle:
        return 0;

    case ResizeWindow:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->resize_window(index, value);

    case GetSampleRate:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->sample_rate;

    case GetBlockSize:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->block_size;

    case GetInputLatency:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->input_latency();

    case GetOutputLatency:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->output_latency();

    case _GetPreviousPlug:
        return 0;

    case _GetNextPlug:
        return 0;

    case _WillReplaceOrAccumulate:
        return 0;

    case GetCurrentProcessLevel:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return (iptr)host->current_process_level();

    case GetAutomationState:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return (iptr)host->automation_state();

    case OfflineStart:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->offline_start(index, value, (Vst::AudioFile*)ptr);

    case OfflineRead:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->offline_read(index, (Vst::OfflineOption*)value,
            (Vst::OfflineTask*)ptr);

    case OfflineWrite:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        // FIXME: Check if this is right.
        return host->offline_write();

    case OfflineGetCurrentPass:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        // FIXME: Check if this is right.
        host->current_offline_pass();
        return 0;

    case OfflineGetCurrentMetaPass:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        // FIXME: Check if this is right.
        host->current_offline_meta_pass();
        return 0;

    case _SetOutputSampleRate:
        return 0;

    case _GetOutputSpeakerArrangement:
        return 0;

    case GetAuthorName: {
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        auto name = host->author_name();
        auto store = (char*)ptr;
        strncpy(store, name.data(), name.size());
        store[name.size()] = '\0';
        return 1;
    }

    case GetHostName: {
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        auto name = host->host_name();
        auto store = (char*)ptr;
        strncpy(store, name.data(), name.size());
        store[name.size()] = '\0';
        return 1;
    }

    case GetVendorVersion:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->vendor_version().full;

    case VendorSpecific:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->vendor_specific(index, value, ptr, opt);

    case _SetIcon:
        return 0;

    case CanDo:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->can_do((char const*)ptr);

    case GetLanguage:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return (iptr)host->language();

    case _OpenWindow:
        return 0;

    case _CloseWindow:
        return 0;

    case GetDirectory:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return (iptr)host->directory();

    case UpdateDisplay:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        host->update_display();
        return 1;

    case BeginEdit:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->begin_parameter_edit(index);

    case EndEdit:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->end_parameter_edit(index);

    case OpenFileSelector:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        return host->open_file_selector((Vst::FileSelect*)ptr);

    case CloseFileSelector:
        if (host == nullptr) {
            LOG("Plugin did not provide itself in dispatch, but it is needed");
            return 0;
        }
        host->close_file_selector((Vst::FileSelect*)ptr);
        return 1;

    case _EditFile:
        return 0;

    case _GetChunkFile:
        return 0;

    case _GetInputSpeakerArrangement:
        return 0;

    }
}

void Host::update_time_info(u32)
{
}

bool Host::begin_parameter_edit(i32 parameter_id)
{
    (void)parameter_id;
    return false;
}

bool Host::automate_parameter(i32 parameter_id, f32 value)
{
    (void)parameter_id;
    (void)value;
    return false;
}

bool Host::end_parameter_edit(i32 parameter_id)
{
    (void)parameter_id;
    return false;
}

i32 Host::current_shell_id() const
{
    return 0;
}

void Host::idle() { }

bool Host::want_midi() const
{
    return true;
}

bool Host::process_events(Vst::Events* events)
{
    (void)events;
    return false;
}

bool Host::io_changed()
{
    return false;
}

bool Host::resize_window(i32, i32)
{
    return false;
}

// u32 sample_rate();
// u32 block_size();
u32 Host::input_latency()
{
    return 0;
}

u32 Host::output_latency()
{
    return 0;
}

// FIXME: Separate to all process levels.
Vst::ProcessLevel Host::current_process_level()
{
    auto priority = getpriority(PRIO_MAX, PRIO_PROCESS);
    if (priority == 20)
        return Vst::ProcessLevel::Realtime;
    return Vst::ProcessLevel::GUI;
}

Vst::AutomationState Host::automation_state()
{
    return Vst::AutomationState::Off;
}

bool Host::offline_start(i32 new_audio_files_size, i32 audio_files_size, Vst::AudioFile* audio_files)
{
    (void)new_audio_files_size;
    (void)audio_files_size;
    (void)audio_files;
    return false;
}


bool Host::offline_read(bool read_source, Vst::OfflineOption* value, Vst::OfflineTask* task)
{
    (void)read_source;
    (void)value;
    (void)task;
    return false;
}

bool Host::offline_write()
{
    return false;
}

void Host::current_offline_pass()
{

}

void Host::current_offline_meta_pass()
{

}

iptr Host::vendor_specific(i32 index, iptr value, void* ptr, f32 opt)
{
    (void)index;
    (void)value;
    (void)ptr;
    (void)opt;
    return 0;
}

bool Host::can_do(char const* thing)
{
    if ("sizeWindow"sv == StringView::from_c_string(thing))
        return true;
    return false;
}

#ifdef __apple__
#error "implement me"
FSSpec Host::directory();
#else
char const* Host::directory()
{
    return nullptr;
}
#endif

void Host::update_display()
{

}

bool Host::open_file_selector(Vst::FileSelect*)
{
    return false;
}

void Host::close_file_selector(Vst::FileSelect*)
{

}

}

#include <JR/GuardDefer.h>
#include <JR/Types.h>
#include <Vst/AEffect.h>
#include <Vst/Opcodes.h>
#include <Vst/PluginData.h>
#include <Vst/Rectangle.h>
#include <Vst/Vst.h>
#include <X11/Xlib.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/types.h>
#include <stdio.h>

#include <sys/resource.h> // getpriority

#include <Host.h>
#include <JR/Log.h>
#include <unistd.h>
#include <stdlib.h>

static constexpr bool should_log_jack_messages = true;

static ErrorOr<jack_port_t**> register_input_ports(jack_client_t* client,
    Vst::Effect const* effect)
{
    auto number_of_inputs = effect->number_of_inputs;
    auto inputs = new jack_port_t*[number_of_inputs];
    auto successful_jack_registrations = 0;
    Defer unregister_jack_ports = [&] {
        if (successful_jack_registrations != number_of_inputs) {
            for (i32 i = 0; i < successful_jack_registrations; i++)
                jack_port_unregister(client, inputs[i]);
        }
    };

    for (i32 i = 0; i < number_of_inputs; i++) {
        auto properties = effect->input_properties(i);
        inputs[i] = jack_port_register(client, properties.label,
            JACK_DEFAULT_AUDIO_TYPE,
            JackPortIsInput, 0);
        if (!inputs[i])
            return Error::from_string_literal("no more JACK inputs available");
        successful_jack_registrations++;
    }

    return inputs;
}

static ErrorOr<jack_port_t**> register_output_ports(jack_client_t* client,
    Vst::Effect const* effect)
{
    auto number_of_outputs = effect->number_of_outputs;
    auto outputs = new jack_port_t*[number_of_outputs];
    auto successful_jack_registrations = 0;
    Defer unregister_jack_ports = [&] {
        if (number_of_outputs != successful_jack_registrations) {
            for (i32 i = 0; i < successful_jack_registrations; i++)
                jack_port_unregister(client, outputs[i]);
        }
    };

    for (i32 i = 0; i < number_of_outputs; i++) {
        auto properties = effect->output_properties(i);
        outputs[i] = jack_port_register(client, properties.label,
            JACK_DEFAULT_AUDIO_TYPE,
            JackPortIsOutput, 0);
        if (!outputs[i])
            return Error::from_string_literal("no more JACK outputs available");
        successful_jack_registrations++;
    }
    return outputs;
}

static ErrorOr<void> connect_to_backend_output_ports(jack_client_t* client,
    jack_port_t** inputs, u32 inputs_size)
{
    auto output_ports = jack_get_ports(client, "capture", nullptr,
        JackPortIsPhysical | JackPortIsOutput);
    if (!output_ports)
        return Error::from_string_literal("no physical capture ports");
    Defer free_output_ports = [&] {
        jack_free(output_ports);
    };

    for (i32 i = 0; i < inputs_size; i++) {
        auto port_name = jack_port_name(inputs[i]);
        LOG_IF(should_log_jack_messages, "connect %s -> %s",
            output_ports[i], port_name);
        if (jack_connect(client, output_ports[i], port_name))
            return Error::from_string_literal("could not connect ports");
        // FIXME: disconnect ports on error.
    }
    return {};
}

static ErrorOr<void> connect_to_backend_input_ports(jack_client_t* client,
    jack_port_t** outputs, u32 outputs_size)
{
    auto input_ports = jack_get_ports(client, nullptr, nullptr,
        JackPortIsPhysical | JackPortIsInput);
    if (!input_ports)
        return Error::from_string_literal("no physical playback ports");
    Defer free_input_ports = [&] {
        jack_free(input_ports);
    };

    for (i32 i = 0; i < outputs_size; i++) {
        auto port_name = jack_port_name(outputs[i]);
        LOG_IF(should_log_jack_messages, "connect %s -> %s", port_name,
            input_ports[i]);
        if (jack_connect(client, port_name, input_ports[i]))
            return Error::from_string_literal("could not connect ports");
        // FIXME: disconnect ports on error.
    }

    return {};
}

// FIXME: Handle multiple inputs.
static ErrorOr<jack_port_t*> register_midi_input(jack_client_t* client, 
    Vst::Effect* effect)
{
    auto number_of_inputs = effect->midi_input_channels_size();
    if (!number_of_inputs)
        return nullptr;
    auto* midi_port = jack_port_register(client, "midi_in",
        JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    if (!midi_port)
        return Error::from_string_literal("could not register midi input");

    return midi_port;
}

// FIXME: Handle multiple outputs.
static ErrorOr<jack_port_t*> register_midi_output(jack_client_t* client,
    Vst::Effect* effect)
{
    auto number_of_inputs = effect->midi_output_channels_size();
    if (!number_of_inputs)
        return nullptr;
    auto* midi_port = jack_port_register(client, "midi_out",
        JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
    if (!midi_port)
        return Error::from_string_literal("could not register midi input");

    return midi_port;
}

ErrorOr<void> connect_to_backend_midi_output_port(jack_client_t* client,
    jack_port_t* midi_input_port)
{
    auto output_ports = jack_get_ports(client, "midi", nullptr,
         JackPortIsPhysical | JackPortIsOutput);
    if (!output_ports)
        return Error::from_string_literal("no physical capture ports");
    Defer free_output_ports = [&] {
        jack_free(output_ports);
    };

    auto port_name = jack_port_name(midi_input_port);
    LOG_IF(should_log_jack_messages, "connect midi %s -> %s",
        output_ports[0], port_name);
    if (jack_connect(client, output_ports[0], port_name))
        return Error::from_string_literal("could not connect midi port");
    // FIXME: disconnect ports on error.

    return {};
}

static int process_audio(u32 number_of_samples, void* argument);
static void on_shutdown(void* argument);

ErrorOr<Host*> Host::create(Vst::Effect* effect)
{
    auto client_name = effect->name();
    jack_status_t status;
    auto client = jack_client_open(client_name, JackNullOption, &status);
    if (!client) {
        if (status & JackServerFailed)
            return Error::from_string_literal("Unable to connect to JACK server");
        return Error::from_string_literal("jack_client_open() failed");
    }
    GuardDefer close_jack_client = [&] {
        jack_client_close(client);
    };
    if (status & JackServerStarted) {
        LOG_IF(should_log_jack_messages, "JACK server started");
    }
    if (status & JackNameNotUnique) {
        client_name = jack_get_client_name(client);
        LOG_IF(should_log_jack_messages,
            "client name was not unique, assigned it '%s'",
            client_name);
    }

    auto number_of_inputs = effect->number_of_inputs;
    auto number_of_outputs = effect->number_of_outputs;
    auto sample_rate = jack_get_sample_rate(client);
    auto host = new Host {
        .client = client,
        .effect = effect,
        .input_buffers = new f32*[number_of_inputs],
        .output_buffers = new f32*[number_of_outputs],
        .number_of_inputs = effect->number_of_inputs,
        .number_of_outputs = effect->number_of_outputs,
        .sample_rate = sample_rate
    };

    jack_set_process_callback(client, process_audio, host);
    jack_on_shutdown(client, on_shutdown, host);
    jack_activate(client);

    auto input_ports = TRY(register_input_ports(client, effect));
    // FIXME: GuardDefer unregister inputs.
    host->input_ports = input_ports;
    (void)(connect_to_backend_output_ports(client, input_ports, number_of_inputs));
    // FIXME: GuardDefer disconnect inputs.

    auto output_ports = TRY(register_output_ports(client, effect));
    // FIXME GuardDefer unregister outputs.
    host->output_ports = output_ports;
    (void)(connect_to_backend_input_ports(client, output_ports, number_of_outputs));
    // FIXME GuardDefer disconnect outputs.
    
    // FIXME: Handle multiple inputs.
    auto midi_input_port = TRY(register_midi_input(client, effect));
    host->midi_input_port = midi_input_port;
    // FIXME: GuardDefer disconnect inputs.
    if (midi_input_port)
        TRY(connect_to_backend_midi_output_port(client, midi_input_port));

    // FIXME: Handle multiple outputs.
    host->midi_output_port = TRY(register_midi_output(client, effect));
    // FIXME: GuardDefer disconnect outputs.

    close_jack_client.disarm();
    host->jack_client_is_ready = true;
    return host;
}

void Host::handle_midi_input_events(u32 number_of_samples) const
{
    constexpr auto should_log_midi_inputs = true;
    if (has_midi_input_port()) {
        auto port_buffer = jack_port_get_buffer(midi_input_port, number_of_samples);
        auto event_count = jack_midi_get_event_count(port_buffer);
        if (event_count > 0)
            LOG_IF(should_log_midi_inputs, "midi events: %d", event_count);
        for (i32 i = 0; i < event_count; i++) {
            jack_midi_event_t midi_event;
            jack_midi_event_get(&midi_event, port_buffer, i);
            u8 byte1 = midi_event.buffer[0];
            u8 byte2 = midi_event.buffer[1];
            u8 byte3 = midi_event.buffer[2];
            LOG_IF(should_log_midi_inputs, "midi packet: %.2x %.2x %.2x",
                    byte1, byte2, byte3);
            send_midi_packet({byte1, byte2, byte3});
        }
    }
}

void Host::destroy() const
{
    jack_client_close(client);
}

f32 const* const* Host::input_samples(u32 number_of_samples)
{
    for (i32 i = 0; i < number_of_inputs; i++) {
        input_buffers[i] = (f32*)jack_port_get_buffer(input_ports[i],
            number_of_samples);
    }
    return input_buffers;
}

f32* const* Host::output_samples(u32 number_of_samples)
{
    for (i32 i = 0; i < number_of_outputs; i++) {
        auto output_port = output_ports[i];
        output_buffers[i] = (f32*)jack_port_get_buffer(output_port,
            number_of_samples);
    }
    return output_buffers;
}

static int process_audio(u32 number_of_samples, void* argument)
{
    auto plugin = (Host*)argument;
    if (!plugin->jack_client_is_ready)
        return 0;
    auto effect = plugin->effect;
    if (!plugin->effect_is_initialized) {
        plugin->effect_is_initialized = true;
        (void)effect->set_block_size(number_of_samples);
    }

    plugin->handle_midi_input_events(number_of_samples);

    auto inputs = plugin->input_samples(number_of_samples);
    auto outputs = plugin->output_samples(number_of_samples);

    effect->process_f32(effect, inputs, outputs, number_of_samples);
    return 0;
}

static void on_shutdown(void* argument)
{
    auto plugin = (Host*)argument;
    (void)plugin;
    // FIXME: Investigate plugin->destroy() from here.
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

intptr_t Host::dispatch(Vst::Effect* effect, Vst::HostOpcode opcode,
    i32 index, intptr_t value, void* ptr, f32 opt)
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
        return host->automate_parameter(index, opt);

    case VstVersion:
        return Host::vst_version();

    case CurrentId:
        return host->current_shell_id();

    case Idle:
        host->idle();
        return 1;

    case _PinConnected:
        return 0;

    case _WantMidi:
        return host->want_midi();

    case GetTime:
        host->update_time_info(index);
        return (intptr_t)&host->time_info;

    case ProcessEvents:
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
        return host->io_changed();

    case _NeedIdle:
        return 0;

    case ResizeWindow:
        return host->resize_window(index, value);

    case GetSampleRate:
        return host->sample_rate;

    case GetBlockSize:
        return host->block_size;

    case GetInputLatency:
        return host->input_latency();

    case GetOutputLatency:
        return host->output_latency();

    case _GetPreviousPlug:
        return 0;

    case _GetNextPlug:
        return 0;

    case _WillReplaceOrAccumulate:
        return 0;

    case GetCurrentProcessLevel:
        return (intptr_t)host->current_process_level();

    case GetAutomationState:
        return (intptr_t)host->automation_state();

    case OfflineStart:
        return host->offline_start(index, value, (Vst::AudioFile*)ptr);

    case OfflineRead:
        return host->offline_read(index, (Vst::OfflineOption*)value,
            (Vst::OfflineTask*)ptr);

    case OfflineWrite:
        // FIXME: Check if this is right.
        return host->offline_write();

    case OfflineGetCurrentPass:
        // FIXME: Check if this is right.
        host->current_offline_pass();
        return 0;

    case OfflineGetCurrentMetaPass:
        // FIXME: Check if this is right.
        host->current_offline_meta_pass();
        return 0;

    case _SetOutputSampleRate:
        return 0;

    case _GetOutputSpeakerArrangement:
        return 0;

    case GetAuthorName: {
        auto name = host->author_name();
        auto store = (char*)ptr;
        strncpy(store, name.data(), name.size());
        store[name.size()] = '\0';
        return 1;
    }

    case GetHostName: {
        auto name = host->host_name();
        auto store = (char*)ptr;
        strncpy(store, name.data(), name.size());
        store[name.size()] = '\0';
        return 1;
    }

    case GetVendorVersion:
        return host->vendor_version().full;

    case VendorSpecific:
        return host->vendor_specific(index, value, ptr, opt);

    case _SetIcon:
        return 0;

    case CanDo:
        return host->can_do((char const*)ptr);

    case GetLanguage:
        return (intptr_t)host->language();

    case _OpenWindow:
        return 0;

    case _CloseWindow:
        return 0;

    case GetDirectory:
        return (intptr_t)host->directory();

    case UpdateDisplay:
        host->update_display();
        return 1;

    case BeginEdit:
        return host->begin_parameter_edit(index);

    case EndEdit:
        return host->end_parameter_edit(index);

    case OpenFileSelector:
        return host->open_file_selector((Vst::FileSelect*)ptr);

    case CloseFileSelector:
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

void Host::update_time_info(u32 request_mask)
{
    (void)request_mask;
    Vst::TimeInfo time;
    auto frame_size = 256;
    auto frames_since_cycle_start = jack_frames_since_cycle_start(client);
    time.sample_position = frames_since_cycle_start * frame_size;
    time.time_signature_numerator = 4;
    time.time_signature_denominator = 4;
    time.system_time_in_nanoseconds = jack_get_time() * 1000.0;
    // clang-format off
    time.flags = (i32)Vst::TimeInfoFlags::CyclePosIsValid
               | (i32)Vst::TimeInfoFlags::NanosecondsIsValid
               ;
    // clang-format on

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

bool Host::resize_window(i32 width, i32 height)
{
    return window->resize(width, height);
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

intptr_t Host::vendor_specific(i32 index, intptr_t value, void* ptr, f32 opt)
{
    (void)index;
    (void)value;
    (void)ptr;
    (void)opt;
    return 0;
}

bool Host::can_do(char const* thing)
{
    constexpr bool log_can_do = true;
    LOG_IF(log_can_do, "can do '%s'?", thing);
    if (StringView("sizeWindow") == thing)
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

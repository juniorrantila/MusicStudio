#include <Midi/Note.h>
#include <Midi/Packet.h>
#include <Vst/Vst.h>

#include <JR/Defer.h>
#include <JR/Log.h>
#include <dlfcn.h>
#include <stdio.h>

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <GUI/Window.h>
#include <Plugin.h>
#include <JR/Defer.h>
#include <JR/ErrorOr.h>

#include <fcntl.h>

template <typename T>
T notify_if_error(ErrorOr<T> result)
{
    if (!result.is_error())
        return result.release_value();
    auto error = result.release_error();
    auto function_name = error.function_name();
    auto message = error.message();
    auto filename = error.filename();
    fprintf(stderr, "Notice: %*s: %*s [%*s:%d]\n",
            function_name.size(), function_name.data(),
            message.size(), message.data(),
            filename.size(), filename.data(),
            error.line_in_file());

    return T();
}

ErrorOr<Midi::Note> keyboard_key_to_midi_note(GUI::Key key)
{
    using enum GUI::Key;
    switch (key) {
    case Number1:
        return Midi::Note::FS4;
    case Number2:
        return Midi::Note::GS4;
    case Number3:
        return Midi::Note::AS4;
    case Number4:
        break;
    case Number5:
        return Midi::Note::CS5;
    case Number6:
        return Midi::Note::DS5;
    case Number7:
        break;
    case Number8:
        return Midi::Note::FS5;
    case Number9:
        return Midi::Note::GS5;
    case Number0:
        return Midi::Note::AS5;

    case Q:
        return Midi::Note::E4;
    case W:
        return Midi::Note::F4;
    case E:
        return Midi::Note::G4;
    case R:
        return Midi::Note::A4;
    case T:
        return Midi::Note::B4;
    case Y:
        return Midi::Note::C5;
    case U:
        return Midi::Note::D5;
    case I:
        return Midi::Note::E5;
    case O:
        return Midi::Note::F5;
    case P:
        return Midi::Note::G5;

    case A:
        break;
    case S:
        return Midi::Note::FS3;
    case D:
        return Midi::Note::GS3;
    case F:
        return Midi::Note::AS3;
    case G:
        break;
    case H:
        return Midi::Note::CS4;
    case J:
        return Midi::Note::DS4;
    case K:
        break;
    case L:
        return Midi::Note::FS4;

    case Z:
        return Midi::Note::F3;
    case X:
        return Midi::Note::G3;
    case C:
        return Midi::Note::A3;
        break;
    case V:
        return Midi::Note::B3;
    case B:
        return Midi::Note::C4;
        break;
    case N:
        return Midi::Note::D4;
    case M:
        return Midi::Note::E4;
    case Comma:
        return Midi::Note::F4;
        break;
    case Period:
        return Midi::Note::G4;
    };
    return Error::from_string_literal("key out of range");
}

namespace MusicStudio {

ErrorOr<int> main(int argc, char const* argv[])
{
    if (argc < 2)
        return fprintf(stderr, "USAGE: %s plugin-path\n", argv[0]), 1;
    auto plugin_path = argv[1];

    auto plugin = TRY(Plugin::create_from(plugin_path));
    Defer destroy_plugin = [&] {
        plugin.destroy();
    };

    auto plugin_name = notify_if_error(plugin.name());
    auto plugin_author = notify_if_error(plugin.author());

    auto plugin_product_version = plugin.product_version();
    auto plugin_version = plugin.version();
    auto plugin_vst_version = plugin.vst_version();

    auto magic_value = plugin.vst_magic();
    char magic_string[sizeof(magic_value) + 1] {};
    magic_string[sizeof(magic_value)] = '\0';
    __builtin_memcpy(magic_string, &magic_value, sizeof(magic_value));

    printf("\n--------------------------------\n");
    printf("       VST Magic: %s (0x%.4X)\n", magic_string, magic_value);
    printf("     VST Version: %d\n\n", plugin_vst_version);
    printf("            Name: %*s\n", plugin_name.size(), plugin_name.data());
    printf("         Version: %d\n", plugin_version);
    printf(" Product version: %d\n", plugin_product_version);
    printf("          Author: %*s\n", plugin_author.size(), plugin_author.data());
    printf(" #    Parameters: %d\n", plugin.number_of_parameters());
    printf(" #       Presets: %d\n", plugin.number_of_presets());
    printf(" #        Inputs: %d\n", plugin.number_of_inputs());
    printf(" #       Outputs: %d\n", plugin.number_of_outputs());
    printf("\n");
    printf("      Has editor: %s\n", plugin.has_editor() ? "yes" : "no");
    printf("    Supports f32: %s\n", plugin.supports_f32() ? "yes" : "no");
    printf("    Supports f64: %s\n", plugin.supports_f64() ? "yes" : "no");
    printf("  Program chunks: %s\n", plugin.uses_program_chunks() ? "yes" : "no");
    printf("        Is synth: %s\n", plugin.is_synth() ? "yes" : "no");
    printf("  Silent stopped: %s\n", plugin.is_silent_when_stopped() ? "yes" : "no");
    printf("----------------------------------\n\n");

#ifdef _WIN32
    printf("Host for windows can't currently view plugin editor.\n");
    return 0;
#endif

    if (!plugin.has_editor()) {
        return 0;
    }

    auto editor_size = plugin.editor_rectangle();
    auto window_name = plugin_name ?: "plugin";
    auto width = editor_size.width ?: 800;
    auto height = editor_size.height ?: 500;
    auto window = TRY(GUI::Window::create(window_name, width, height));
    plugin.host->window = &window;
    Defer destroy_window = [&] {
        window.destroy();
    };
    
    TRY(plugin.open_editor(window.raw_handle()));
    Defer close_editor = [&] {
        plugin.close_editor();
    };

    window.set_on_key_down<Host>(plugin.host, [](Host* host, GUI::Key key) {
        auto packet = Midi::Packet::NoteOn {
            .velocity = 127
        };
        auto note = keyboard_key_to_midi_note(key);
        if (note.has_value()) {
            packet.note = note.release_value();
            host->send_midi_packet(packet);
        }
    });

    window.set_on_key_up<Host>(plugin.host, [](Host* host, GUI::Key key) {
        auto packet = Midi::Packet::NoteOff {
            .velocity = 127
        };
        auto note = keyboard_key_to_midi_note(key);
        if (note.has_value()) {
            packet.note = note.release_value();
            host->send_midi_packet(packet);
        }
    });


    notify_if_error(plugin.set_sample_rate(plugin.host->sample_rate));
    notify_if_error(plugin.resume());

    // Change DPI.
    plugin.vst->vendor_specific(1349674323, 1097155443, nullptr, 1.5);

#if 0
    pthread_t thread;
    pthread_create(&thread, nullptr, [](void* argument) -> void* {
        auto host = (Host*)argument;
        constexpr bool should_log_midi_packet = true;
        auto midi_device = open("/dev/midi1", O_RDONLY);
        while (1) {
            u8 midi_buffer[4];
            read(midi_device, midi_buffer, sizeof(midi_buffer));
            LOG_IF(should_log_midi_packet, "midi packet: %.2x %.2x %.2x",
                midi_buffer[0], midi_buffer[1], midi_buffer[2]);
            host->send_midi_packet({midi_buffer[0],
                                    midi_buffer[1],
                                    midi_buffer[2]});
        }

        if (host->has_midi_input_port())
            host->handle_midi_input_events_loop();
        
        return nullptr;
    }, plugin.host);
#endif

    while(!window.should_close()) { }

    return 0;
}

}


int main(int argc, char const* argv[])
{
    auto result = MusicStudio::main(argc, argv);
    if (result.is_error()) {
        auto error = result.release_error();
        auto function_name = error.function_name();
        auto message = error.message();
        auto filename = error.filename();
        fprintf(stderr, "Error: %*s: %*s [%*s:%d]\n",
                function_name.size(), function_name.data(),
                message.size(), message.data(),
                filename.size(), filename.data(),
                error.line_in_file());
        return -1;
    }
    return 0;
}

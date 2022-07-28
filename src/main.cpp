#include <Midi/Note.h>
#include <Midi/Packet.h>
#include <SDL2/SDL_thread.h>
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
#include <MainWindow.h>

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

    SDL_SetHint(SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT, "true");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0)
        return Error::from_string_literal(SDL_GetError());
    Defer quit_sdl = [&] {
        SDL_Quit();
    };

    auto main_window = TRY(MainWindow::create("MusicStudio", 1600, 900));
    Defer destroy_main_window = [&] {
        main_window.destroy();
    };

    auto main_gui_thread = SDL_CreateThread([](void* argument) -> i32 {
        auto& main_window = *(MainWindow*)argument;
        main_window.show();
        bool should_quit = false;
        while (!should_quit) {
            ImGui::SetCurrentContext(main_window.imgui.context);

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                main_window.imgui.process_event(&event);
                if (event.type == SDL_QUIT)
                    should_quit = true;
                if (event.type == SDL_KEYUP) {
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                        should_quit = true;
                }
            }

            main_window.imgui.begin_frame();
            Defer end_frame = [&] {
                main_window.imgui.end_frame();
            };

            ImGui::Text("Hello World!");
        }

        return 0;
    }, "MainWindow", &main_window);

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

    SDL_WaitThread(main_gui_thread, nullptr);

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

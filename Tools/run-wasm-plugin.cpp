#include <Core/Print.h>
#include <CLI/ArgumentParser.h>
#include <MS/WASMPlugin.h>
#include <Main/Main.h>

ErrorOr<int> Main::main(int argc, char const* argv[])
{
    auto argument_parser = CLI::ArgumentParser();

    auto plugin_path = StringView();
    TRY(argument_parser.add_positional_argument("plugin-path"sv, [&](c_string arg) {
        plugin_path = StringView::from_c_string(arg);
    }));

    bool process_f32 = false;
    TRY(argument_parser.add_flag("--process-f32"sv, "-p32", "process 16 f32 frames in stereo", [&] {
        process_f32 = true;
    }));

    bool process_f64 = false;
    TRY(argument_parser.add_flag("--process-f64"sv, "-p64", "process 16 f64 frames in stereo", [&] {
        process_f64 = true;
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    auto plugin = TRY(MS::WASMPlugin::create(plugin_path));
    TRY(plugin.link());
    TRY(plugin.run());

    constexpr u32 channel_count = 2;
    constexpr u32 frame_count = 16;

    f32 samples_f32[channel_count * frame_count];
    if (process_f32) {
        TRY(plugin.process_f32(samples_f32, nullptr, frame_count, channel_count));
        dprintln("Process f32:");
        for (u32 frame = 0; frame < frame_count; frame++) {
            for (u32 channel = 0; channel < channel_count; channel++) {
                usize index = frame * channel_count + channel;
                dprint("{} ", samples_f32[index]);
            }
            dprintln("");
        }
    }

    f64 samples_f64[channel_count * frame_count];
    if (process_f64) {
        TRY(plugin.process_f64(samples_f64, nullptr, frame_count, channel_count));

        dprintln("Process f64:");
        for (u32 frame = 0; frame < frame_count; frame++) {
            for (u32 channel = 0; channel < channel_count; channel++) {
                usize index = frame * channel_count + channel;
                dprint("{} ", samples_f64[index]);
            }
            dprintln("");
        }
    }

    return 0;
}

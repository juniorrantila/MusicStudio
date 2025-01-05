#include <Core/Print.h>
#include <CLI/ArgumentParser.h>
#include <MS/WASMPluginManager.h>
#include <Main/Main.h>
#include <MS/Project.h>
#include <MS/WASMPlugin/Plugin.h>

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

    bool dump_info = false;
    TRY(argument_parser.add_flag("--dump-info"sv, "-di", "dump plugin info", [&] {
        dump_info = true;
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    auto project = MS::Project {
        .sample_rate = 44100,
        .channels = 2,
    };

    auto manager = MS::WASMPluginManager(Ref(project));
    auto plugin_id = TRY(manager.add_plugin(plugin_path));
    TRY(manager.link());
    TRY(manager.init());
    Defer deinit_plugin = [&]{
        manager.deinit().or_else([](Error error) {
            dprintln("Error: could not deinit plugin manager: ", error);
        });
    };
    auto const* plugin = manager.plugin(plugin_id);

    if (dump_info) {
        dprintln("info:");
        dprintln("  can process f32: {}", plugin->can_process_f32());
        dprintln("  can process f64: {}", plugin->can_process_f64());
        dprintln("  parameter count: {}", plugin->parameter_count());
        plugin->parameter_count().then([&](u32 count) {
            dprintln("  parameters:");
            for (u32 param = 0; param < count; param++) {
                dprintln("    {}:", param);
                dprintln("      name: {}", plugin->parameter_name(param));
                dprintln("      kind: {}", plugin->parameter_kind(param));
                dprintln("      min value: {}", plugin->parameter_min_value(param));
                dprintln("      max value: {}", plugin->parameter_max_value(param));
                dprintln("      step size: {}", plugin->parameter_step_size(param));
                if (plugin->parameter_kind(param) == MSPluginParameterKind_Options) {
                    dprintln("      options:");
                    u32 min_value = (u32)plugin->parameter_min_value(param).or_default(0);
                    u32 max_value = (u32)plugin->parameter_max_value(param).or_default(0);
                    u32 step_size = (u32)plugin->parameter_step_size(param).or_default(0);
                    for (u32 option_id = min_value; option_id <= max_value; option_id += step_size) {
                        dprintln("        {}: {}", option_id, plugin->parameter_option_name(param, option_id));
                    }
                }
            }
        });
    }

    constexpr u32 channel_count = 2;
    constexpr u32 frame_count = 16;

    f32 samples_f32[channel_count * frame_count];
    if (process_f32) {
        TRY(plugin->process_f32(samples_f32, nullptr, frame_count, channel_count));
        dprintln("process f32:");
        for (u32 frame = 0; frame < frame_count; frame++) {
            for (u32 channel = 0; channel < channel_count; channel++) {
                usize index = frame * channel_count + channel;
                dprint("  {} ", samples_f32[index]);
            }
            dprintln("");
        }
    }

    f64 samples_f64[channel_count * frame_count];
    if (process_f64) {
        TRY(plugin->process_f64(samples_f64, nullptr, frame_count, channel_count));

        dprintln("process f64:");
        for (u32 frame = 0; frame < frame_count; frame++) {
            for (u32 channel = 0; channel < channel_count; channel++) {
                usize index = frame * channel_count + channel;
                dprint("  {} ", samples_f64[index]);
            }
            dprintln("");
        }
    }

    return 0;
}

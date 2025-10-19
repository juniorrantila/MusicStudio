#include <LibCLI/ArgumentParser.h>
#include <LibMusic/VstPlugin.h>
#include <LibTy/Defer.h>
#include <LibTy/ErrorOr.h>
#include <LibUI/Application.h>
#include <LibUI/Window.h>
#include <LibVST/Rectangle.h>
#include <LibVST/Vst.h>
#include <LibMain/Main.h>

#include <stdio.h>
#include <libgen.h>

namespace Main {

ErrorOr<int> main(int argc, char const* argv[])
{
    auto argument_parser = CLI::ArgumentParser();

    c_string plugin_path = nullptr;
    TRY(argument_parser.add_positional_argument("plugin-path"sv, [&](c_string arg) {
        plugin_path = arg;
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    auto plugin = TRY(MS::Plugin::create_from(plugin_path));
    Defer destroy_plugin = [&] {
        plugin.destroy();
    };

    auto plugin_name = plugin.name().or_else(StringView::from_c_string(basename((char*)plugin_path)));
    auto plugin_author = plugin.author().or_else("<none>"sv);

    auto plugin_product_version = plugin.product_version();
    auto plugin_version = plugin.version();
    auto plugin_vst_version = plugin.vst_version();

    auto magic_value = plugin.vst_magic();
    printf("\n--------------------------------\n");
    printf("       VST Magic: %.4s (0x%.4X)\n", (char*)&magic_value, magic_value);
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

    auto plugin_name_c_string = TRY(StringBuffer::create_fill(plugin_name, "\0"sv));

    auto rect = plugin.editor_rectangle().or_else(Vst::Rectangle{ 0, 0, 800, 600 });

    auto* app = ui_application_create(UIApplicationHint_NativeLike);
    Defer destroy_app = [&] {
        ui_application_destroy(app);
    };

    auto* window = ui_window_create(app, {
        .parent = nullptr,
        .title = plugin_name_c_string.data(),
        .x = rect.x,
        .y = rect.y,
        .width = rect.width,
        .height = rect.height,
    });

    void* window_handle = ui_window_native_handle(window);
    if (!plugin.open_editor(window_handle)) {
        return Error::from_string_literal("could not open editor");
    }

    while (!ui_window_should_close(window)) {
        ui_application_poll_events(app);
    }

    return 0;
}

}

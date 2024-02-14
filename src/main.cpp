#include <CLI/ArgumentParser.h>
#include <Midi/Note.h>
#include <Midi/Packet.h>
#include <MS/Plugin.h>
#include <Ty/Defer.h>
#include <Ty/Defer.h>
#include <Ty/ErrorOr.h>
#include <Vst/Vst.h>

#include <stdio.h>

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

    auto plugin_name = TRY(plugin.name());
    auto plugin_author = TRY(plugin.author());

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

    if (!plugin.has_editor()) {
        return 0;
    }

    TRY(plugin.set_sample_rate(plugin.host->sample_rate));
    TRY(plugin.resume());

    // Change DPI.
    plugin.vst->vendor_specific(1349674323, 1097155443, nullptr, 1.5);

    return 0;
}

}

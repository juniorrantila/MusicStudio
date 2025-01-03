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

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    auto plugin = TRY(MS::WASMPlugin::create(plugin_path));
    TRY(plugin.link());
    TRY(plugin.run());

    return 0;
}

#include "./Application.h"

#include <FS/Bundle.h>
#include <CLI/ArgumentParser.h>
#include <Core/Print.h>
#include <Main/Main.h>
#include <Fonts/Fonts.h>
#include <UI/Shaders/Shaders.h>

ErrorOr<int> Main::main(int argc, c_string argv[])
{
    auto bundle = FS::Bundle()
        .add_pack(Fonts())
        .add_pack(UI::Shaders());

    dbgln("resources:"sv);
    for (auto resource : bundle.resources()) {
        dbgln("  "sv, resource.resolved_path());
    }

    auto argument_parser = CLI::ArgumentParser();

    auto root_directory = "."sv;
    TRY(argument_parser.add_option("--root", "-r", "directory", "root directory", [&](c_string arg) {
        root_directory = StringView::from_c_string(arg);
    }));

    auto plugin_paths = Vector<StringView>();
    TRY(argument_parser.add_option("--plugin", "-p", "plugin", "open plugin", [&](c_string arg) {
        MUST(plugin_paths.append(StringView::from_c_string(arg)));
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    auto application = TRY(Application::create(bundle));

    for (auto plugin : plugin_paths) {
        TRY(application.open_plugin(plugin));
    }
    TRY(application.change_path(root_directory));

    application.run();
    return 0;
}

#include "Ty/System.h"
#include <Core/Print.h>
#include <FS/Bundle.h>
#include <Main/Main.h>
#include <CLI/ArgumentParser.h>
#include <Core/File.h>

ErrorOr<int> Main::main(int argc, c_string argv[])
{
    auto argument_parser = CLI::ArgumentParser();

    auto output_path = Optional<StringView>();
    TRY(argument_parser.add_option("--output", "-o", "path", "output path", [&](c_string  arg) {
        output_path = StringView::from_c_string(arg);
    }));

    StringView c_variable_name = ""sv;
    TRY(argument_parser.add_option("--generate-c-code", "-i", "c-variable-name", "generate c code", [&](c_string  arg) {
        c_variable_name = StringView::from_c_string(arg);
    }));

    auto file_paths = Vector<StringView>();
    TRY(argument_parser.add_option("--file", "-f", "from:dest", "e.g. src/foobar.txt:project/foobar.txt", [&](c_string path){
        MUST(file_paths.append(StringView::from_c_string(path)));
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    auto bundle = FS::Bundle();
    for (auto file : file_paths) {
        auto chunks = TRY(file.split_on(":"sv));
        if (chunks.size() < 2) {
            return Error::from_string_literal("expected --file format to be src:dest, see --help");
        }
        auto source = chunks[0];
        auto dest = chunks[1];
        TRY(bundle.mount(source, dest));
    }
    auto bytes = TRY(bundle.bytes());
    if (c_variable_name.is_empty()) {
        if (!output_path.has_value()) {
            output_path = "Bundle.zip"sv;
        }
        auto file = TRY(Core::File::open_for_writing(*output_path, O_TRUNC));
        TRY(file.write(bytes.as_view()));
        return 0;
    }

    auto c_source_file = TRY(bytes.as_c_source_file(c_variable_name));
    if (output_path.has_value()) {
        auto file = TRY(Core::File::open_for_writing(*output_path, O_TRUNC));
        TRY(file.write(c_source_file.view()));
        return 0;
    }
    outwrite(c_source_file.view());

    return 0;
}

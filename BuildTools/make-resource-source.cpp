#include <CLI/ArgumentParser.h>
#include <Core/File.h>
#include <Core/MappedFile.h>
#include <Core/Print.h>
#include <Main/Main.h>
#include <stdio.h>
#include <stdlib.h>

struct CurrentLocation {
    constexpr CurrentLocation(StringView file = __builtin_FILE(), u32 line = __builtin_LINE())
        : file(file)
        , line(line)
    {
    }

    StringView file {};
    u32 line { 0 };
};

ErrorOr<int> Main::main(int argc, c_string argv[])
{
    auto argument_parser = CLI::ArgumentParser();

    auto root = StringView();
    TRY(argument_parser.add_positional_argument("root-path", [&](c_string arg) {
        root = StringView::from_c_string(arg);
    }));

    auto input = StringView();
    TRY(argument_parser.add_positional_argument("input", [&](c_string arg) {
        input = StringView::from_c_string(arg);
    }));

    auto output = StringView();
    TRY(argument_parser.add_positional_argument("output", [&](c_string arg) {
        output = StringView::from_c_string(arg);
    }));

    auto build_dir_path = StringView();
    TRY(argument_parser.add_positional_argument("build-dir", [&](c_string arg) {
        build_dir_path = StringView::from_c_string(arg);
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    auto input_file = TRY(Core::MappedFile::open(input));

    auto file_path = TRY(input.resolve_path(root));
    auto normalized_path = TRY(StringBuffer::create_fill(file_path.view()));
    for (u8 i = 0; i < 128; i++) {
        if (i >= 'a' && i <= 'z')
            continue;
        if (i >= 'A' && i <= 'Z') {
            normalized_path.replace_all(i, i - 'A' + 'a');
            continue;
        }
        if (i >= '0' && i <= '9')
            continue;
        if (i == '_')
            continue;
        normalized_path.replace_all(i, '_');
    }

    {
        auto path = TRY(StringBuffer::create_fill(output, ".name"sv));
        auto file = TRY(Core::File::open_for_writing(path.view(), O_TRUNC));
        TRY(file.writeln(normalized_path.view()));
    }

    {
        auto file = TRY(Core::File::open_for_writing(output, O_CREAT|O_TRUNC));
        TRY(file.writeln("// Generated with make-resource-source "sv));
        TRY(file.writeln("// root:   \""sv, root, "\""sv));
        TRY(file.writeln("// input:  \""sv, input, "\""sv));
        TRY(file.writeln("// output: \""sv, output, "\""sv));
        TRY(file.writeln("// build_dir: \""sv, build_dir_path, "\""sv));
        TRY(file.writeln());
        TRY(file.writeln("#pragma clang diagnostic ignored \"-Weverything\""sv, CurrentLocation()));
        TRY(file.writeln());
        TRY(file.writeln("_Static_assert(sizeof(unsigned char) == 1, \"just making sure\");"sv, CurrentLocation()));
        TRY(file.writeln("const unsigned char __"sv, normalized_path.view(), "_bytes[1 + "sv, input_file.size(), "ULL] ="sv, CurrentLocation()));

        TRY(file.write("    \""sv));
        usize printed_chars = 0;
        usize row_break = 80;
        for (usize i = 0; i < input_file.size(); i++) {
            unsigned char c = (unsigned char)input_file.data()[i];

            if (printed_chars >= row_break) {
                printed_chars = TRY(file.write("\"\n    \""sv));
            }

            if (c == '"') {
                printed_chars += TRY(file.write("\\\""sv));
            } else if (c == '\n') {
                printed_chars += TRY(file.write("\\n"sv));
            } else if (c == '\r'){
                printed_chars += TRY(file.write("\\r"sv));
            } else if (c == '\\') {
                printed_chars += TRY(file.write("\\\\"sv));
            } else [[likely]] {
                printed_chars += TRY(file.write(StringView::from_parts((char*)&c, 1)));
            }
        }
        TRY(file.writeln("\";\n"sv));
    }

    return 0;
}

template <>
struct Ty::Formatter<CurrentLocation> {
    template <typename U>
        requires Ty::Writable<U>
    static constexpr ErrorOr<u32> write(U& to, CurrentLocation location)
    {
        return TRY(to.write(" // "sv, location.file, ":"sv, location.line));
    }
};

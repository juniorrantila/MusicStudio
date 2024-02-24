#include <CLI/ArgumentParser.h>
#include <Core/File.h>
#include <Core/MappedFile.h>
#include <Core/Print.h>
#include <Main/Main.h>
#include <stdio.h>

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
    TRY(argument_parser.add_positional_argument("input-file", [&](c_string arg) {
        input = StringView::from_c_string(arg);
    }));

    auto output = StringView();
    TRY(argument_parser.add_positional_argument("output-cpp", [&](c_string arg) {
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
        TRY(file.writeln("// Generated with bundle "sv));
        TRY(file.writeln("// root:   \""sv, root, "\""sv));
        TRY(file.writeln("// input:  \""sv, input, "\""sv));
        TRY(file.writeln("// output: \""sv, output, "\""sv));
        TRY(file.writeln("// build_dir: \""sv, build_dir_path, "\""sv));
        TRY(file.writeln());
        TRY(file.writeln("#include <Bundle/Resource.h> "sv, CurrentLocation()));
        TRY(file.writeln());
        TRY(file.writeln("static u8 "sv, normalized_path.view(), "_bytes[] = { "sv, CurrentLocation()));

        auto file_bytes = Bytes(input_file.data(), input_file.size());
        u32 numbers_per_line = 8;
        for (u32 y = 0; y < file_bytes.size() / numbers_per_line; y++) {
            TRY(file.write("        "sv));
            for (u32 x = 0; x < numbers_per_line; x++) {
                u32 i = y * numbers_per_line + x;
                Bytes byte = Bytes(&file_bytes[i], 1);
                TRY(file.write("0x"sv, byte, ", "sv));
            }
            TRY(file.writeln(CurrentLocation()));
        }
        if (file_bytes.size() % numbers_per_line != 0) {
            u32 last_bytes = file_bytes.size() % numbers_per_line;
            u32 start = file_bytes.size() - last_bytes;
            TRY(file.write("        "sv));
            for (u32 i = 0; i < last_bytes; i++) {
                auto byte = Bytes(&file_bytes[start + i], 1);
                TRY(file.write("0x"sv, byte, ", "sv));
            }
            TRY(file.writeln(CurrentLocation()));
        }

        TRY(file.writeln("}; "sv, CurrentLocation()));

        TRY(file.writeln("static inline const Bundle::Resource "sv, normalized_path.view(), " = Bundle::Resource::create_with_resolved_path("sv, CurrentLocation()));
        TRY(file.writeln("    \""sv, file_path.view(), "\", "sv, CurrentLocation()));

        auto path = normalized_path.view();
        TRY(file.writeln("    Bytes("sv, path, "_bytes, "sv, "sizeof("sv, path, "_bytes)) "sv, CurrentLocation()));
        TRY(file.writeln("); "sv, CurrentLocation()));
    }

    return 0;
}

template <>
struct Ty::Formatter<CurrentLocation> {
    template <typename U>
        requires Ty::Writable<U>
    static constexpr ErrorOr<u32> write(U& to, CurrentLocation location)
    {
        return TRY(to.write("// "sv, location.file, ":"sv, location.line));
    }
};

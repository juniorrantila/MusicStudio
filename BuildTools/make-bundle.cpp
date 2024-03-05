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
    if (argc < 2)
        return 1;

    auto output_path = StringView::from_c_string(argv[1]);

    auto file_paths = TRY(Vector<StringView>::create(argc - 1));
    for (int i = 2; i < argc; i++) {
        TRY(file_paths.append(StringView::from_c_string(argv[i])));
    }

    auto output_file = TRY(Core::File::open_for_writing(output_path, O_TRUNC));

    TRY(output_file.writeln("// Generated with make-bundle"sv));
    for (int i = 0; i < argc; i++) {
        auto arg = StringView::from_c_string(argv[i]);
        TRY(output_file.writeln("// "sv, arg));
    }
    TRY(output_file.writeln());
    TRY(output_file.writeln("#include <Bundle/Bundle.h> "sv, CurrentLocation()));
    TRY(output_file.writeln());

    auto desc_file_paths = TRY(Vector<StringBuffer>::create(file_paths.size()));
    for (auto path : file_paths) {
        if (!path.ends_with(".h"sv))
            continue;
        auto resolved_path = TRY(path.resolve_path());
        path = resolved_path.view();
        TRY(output_file.writeln("#include \"./"sv, path, "\" "sv, CurrentLocation()));

        TRY(desc_file_paths.append(TRY(StringBuffer::create_fill(path, ".name"sv))));
    }
    TRY(output_file.writeln());

    TRY(output_file.writeln("namespace Bundle { "sv, CurrentLocation()));
    TRY(output_file.writeln());

    TRY(output_file.writeln("Bundle& Bundle::the() { "sv, CurrentLocation()));
    TRY(output_file.writeln("    static auto bundle = Bundle(); "sv, CurrentLocation()));
    TRY(output_file.writeln("    static bool is_initialized = false; "sv, CurrentLocation()));
    TRY(output_file.writeln("    if (is_initialized) return bundle; "sv, CurrentLocation()));
    for (auto const& path : desc_file_paths) {
        auto desc_file = TRY(Core::MappedFile::open(path.view()));
        auto resource = desc_file.view().shrink("\n"sv.size());
        TRY(output_file.writeln("    bundle.add_resource("sv, resource, "); "sv, CurrentLocation()));
    }
    TRY(output_file.writeln("    return is_initialized = true, bundle; "sv, CurrentLocation()));
    TRY(output_file.writeln("} "sv, CurrentLocation()));

    TRY(output_file.writeln());
    TRY(output_file.writeln("} "sv, CurrentLocation()));

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

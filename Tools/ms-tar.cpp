#include <Core/File.h>
#include <Core/MappedFile.h>
#include <CLI/ArgumentParser.h>
#include <Main/Main.h>
#include <Tar/Tar.h>
#include <Ty/SegmentedArena.h>
#include <Ty/PageAllocator.h>
#include <Core/Print.h>

ErrorOr<int> Main::main(int argc, char const* argv[])
{
    auto argument_parser = CLI::ArgumentParser();

    auto paths = Vector<StringView>();
    TRY(argument_parser.add_option("--add", "-a", "source[:dest]", "add source path to tar at dest", [&](c_string arg) {
        MUST(paths.append(StringView::from_c_string(arg)));
    }));

    auto output_path = "/dev/stdout"sv;
    TRY(argument_parser.add_option("--output", "-o", "path", "output path (default: /dev/stdout)", [&](c_string arg) {
        output_path = StringView::from_c_string(arg);
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    auto output = TRY(Core::File::open_for_writing(output_path));

    // FIXME: Don't use an arena for this.
    auto arena_allocator = segmented_arena_create(page_allocator());
    auto* arena = &arena_allocator.allocator;

    Tar* tar = tar_create(arena);
    if (!tar) {
        dprintln("could not create tar: out of memory");
        return -1;
    }

    for (auto path : paths) {
        auto splits = TRY(path.split_on(':'));
        auto source = TRY(splits.view().at(0).or_error(Error::from_string_literal("invalid source path")));
        auto dest = splits.view().at(1).or_default(source);
        auto content = TRY(Core::MappedFile::open(source));
        auto index = tar_add_borrowed2(tar, dest.data(), dest.size(), content.data(), content.size());
        if (index < 0) {
            auto error = StringView::from_c_string(tar_strerror((e_tar)-index));
            dprintln("could not add '{}:{}' to tar: {}", source, dest, error);
            return -1;
        }
        content.invalidate(); // Leak :)
    }
    auto buffer = TRY(arena->alloc<u8>(tar_buffer_size(tar)));
    if (auto size = tar_buffer(tar, buffer.data(), buffer.size()); size < 0) {
        dprintln("could not create tar file: {}", StringView::from_c_string(tar_strerror((e_tar)-size)));
        return -1;
    }
    TRY(output.write(buffer.as_string()));

    return 0;
}

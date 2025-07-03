#include <Core/File.h>
#include <Core/MappedFile.h>
#include <CLI/ArgumentParser.h>
#include <Main/Main.h>
#include <Tar/Tar.h>
#include <Ty2/FixedArena.h>
#include <Ty2/PageAllocator.h>
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

    TarCounter counter = tar_counter();
    auto files = Vector<Core::MappedFile>();
    auto dest_paths = Vector<StringView>();
    for (auto path : paths) {
        auto splits = TRY(path.split_on(':'));
        auto source = TRY(splits.view().at(0).or_error(Error::from_string_literal("invalid source path")));
        auto dest = splits.view().at(1).or_default(source);
        auto content = TRY(Core::MappedFile::open(source));
        tar_count(&counter, content.size());
        TRY(files.append(move(content)));
        TRY(dest_paths.append(dest));
    }

    void* tar_buf = page_alloc(counter.size);
    if (!tar_buf) {
        dprintln("could not create tar: out of memory");
        return -1;
    }
    auto tar = tar_init(tar_buf, counter.size);
    for (u32 i = 0; i < files.size(); i++) {
        auto content = files[i].view();
        auto path = paths[i];
        if (e_tar error = tar_add2(&tar, path.data(), path.size(), content.data(), content.size()); error != e_tar_none) {
            auto message = StringView::from_c_string(tar_strerror(error));
            dprintln("could not tar file: {}", message);
            return -1;
        }
    }

    auto output = TRY(Core::File::open_for_writing(output_path));
    TRY(output.write(StringView::from_parts((char*)tar_buffer(tar), tar_size(tar))));

    return 0;
}

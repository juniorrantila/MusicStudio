#include <Basic/Allocator.h>
#include <Basic/Base.h>
#include <Basic/Context.h>
#include <Basic/FixedArena.h>
#include <Basic/PageAllocator.h>
#include <Basic/StringSlice.h>

#include <LibCLI/ArgumentParser.h>
#include <LibCore/FSVolume.h>
#include <LibCore/Print.h>
#include <LibMain/Main.h>

#include <stdio.h>

ErrorOr<int> Main::main(int argc, c_string argv[])
{
    constexpr u64 arena_size = 16LLU * 1024LLU * 1024LLU * 1024LLU;
    auto arena_instance = fixed_arena_init(page_alloc(arena_size), arena_size);
    auto* arena = &arena_instance.allocator;

    auto argument_parser = CLI::ArgumentParser();

    const auto max_paths = 16384;
    static StringSlice paths[max_paths];
    usize path_count = 0;
    TRY(argument_parser.add_option("--file", "-f", "path", "watch file at path (can be used multiple times)", [&](c_string arg){
        auto path = sv_from_c_string(arg);
        if (!sv_equal(path, "-"s)) {
            VERIFY(path_count + 1 < max_paths);
            paths[path_count++] = path;
            return;
        }

        char* line = nullptr;
        usize len = 0;
        ssize_t read = 0;
        while ((read = getline(&line, &len, stdin)) != -1) {
            auto path = StringView::from_parts(line, read);
            if (!path.starts_with("../")) {
                continue;
            }
            path = path.chop_left("../"sv.size());
            if (path.ends_with("\n")) {
                path = path.shrink("\n"sv.size());
            }
            VERIFY(path_count + 1 < max_paths);
            char* buf = (char*)memclone(arena, path.data(), path.size(), 1);
            paths[path_count++] = sv_from_parts(buf, path.size());
        }
        free(line);
    }));

    c_string command = nullptr;
    TRY(argument_parser.add_positional_argument("command", [&](c_string arg){
        command = arg;
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    if (path_count == 0) {
        return Error::from_string_literal("no path specified (see --help)");
    }

    FSVolume* volume = (FSVolume*)page_alloc(sizeof(FSVolume));
    fs_volume_init(volume);

    for (usize i = 0; i < path_count; i++) {
        auto path = paths[i];
        FSFile file;
        if (!fs_system_open(arena, path, &file)) {
            dprintln("could not open '{}'", StringView(path));
            return 1;
        }
        if (!fs_volume_mount(volume, file, nullptr)) {
            dprintln("could not mount '{}'", StringView(path));
            return 1;
        }
    }

    while (1) {
        FSEvents events = fs_volume_poll_events(volume, nullptr);
        if (events.count) system(command);
    }

    return 0;
}

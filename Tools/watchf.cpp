#include <Core/Print.h>
#include <Ty/StringSlice.h>
#include <Ty2/Base.h>
#include <Ty2/Allocator.h>
#include <FS/FSVolume.h>
#include <Main/Main.h>
#include <CLI/ArgumentParser.h>
#include <Ty2/PageAllocator.h>
#include <Ty2/Arena.h>

#include <stdio.h>

ErrorOr<int> Main::main(int argc, c_string argv[])
{
    auto arena_instance = arena_create(page_allocator());
    auto* arena = &arena_instance.allocator;

    auto argument_parser = CLI::ArgumentParser();

    const auto max_paths = 16384;
    static StringSlice paths[max_paths];
    usize path_count = 0;
    TRY(argument_parser.add_option("--file", "-f", "path", "watch file at path (can be used multiple times)", [&](c_string arg){
        auto path = string_slice_from_c_string(arg);
        if (!path.equal("-"s)) {
            VERIFY(path_count + 1 < max_paths);
            paths[path_count++] = path;
            return;
        }

        char* line = nullptr;
        usize len = 0;
        ssize_t read = 0;
        while ((read = getline(&line, &len, stdin)) != -1) {
            auto path = string_slice(line, read).as_view();
            if (!path.starts_with("../")) {
                continue;
            }
            path = path.chop_left("../"sv.size());
            if (path.ends_with("\n")) {
                path = path.shrink("\n"sv.size());
            }
            VERIFY(path_count + 1 < max_paths);
            char* buf = (char*)memclone(arena, path.data(), path.size(), 1);
            paths[path_count++] = string_slice(buf, path.size());
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

    auto* volume = fs_volume_create(arena);
    if (!volume) return Error::from_string_literal("could not create volume");

    for (usize i = 0; i < path_count; i++) {
        auto path = paths[i];
        FSFile file;
        if (!fs_system_open(arena, path, &file)) {
            dprintln("could not open '{}'", path.as_view());
            return 1;
        }
        if (!fs_volume_mount(volume, file, nullptr)) {
            dprintln("could not mount '{}'", path.as_view());
            return 1;
        }
    }

    while (1) {
        FSEvents events = fs_volume_poll_events(volume, nullptr);
        if (events.count) system(command);
    }

    return 0;
}

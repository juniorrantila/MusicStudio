#include <Core/Print.h>
#include <Ty/StringSlice.h>
#include <Ty2/Base.h>
#include <Ty2/Allocator.h>
#include <FS/FSVolume.h>
#include <Main/Main.h>
#include <CLI/ArgumentParser.h>
#include <Ty2/PageAllocator.h>
#include <Ty2/Arena.h>

ErrorOr<int> Main::main(int argc, c_string argv[])
{
    auto arena_instance = arena_create(page_allocator());
    auto* arena = &arena_instance.allocator;

    auto argument_parser = CLI::ArgumentParser();

    const auto max_paths = 16384;
    static StringSlice paths[max_paths];
    usize path_count = 0;
    TRY(argument_parser.add_option("--file", "-f", "path", "watch file at path (can be used multiple times)", [&](c_string arg){
        VERIFY(path_count + 1 < max_paths);
        paths[path_count++] = string_slice_from_c_string(arg);
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
        for (usize i = 0; i < events.count; i++) {
            FSEvent event = events.items[i];
            auto path = fs_virtual_path(*event.file);
            switch (event.kind) {
            case FSEventKind_Modify: {
                dprintln("modified file: {}", path.as_view());
                if (!fs_file_reload(event.file)) {
                    dprintln("could not load modified content");
                    continue;
                }
                auto content = fs_content(*event.file);
                dprintln("new content:");
                dprintln("{}", content.as_view());
                continue;
            }
            case FSEventKind_Delete:
                dprintln("deleted file: {}", path.as_view());
                continue;
            }
        }
    }

    return 0;
}

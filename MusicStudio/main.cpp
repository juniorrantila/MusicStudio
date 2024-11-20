#include <Main/Main.h>
#include <Core/Print.h>
#include <CLI/ArgumentParser.h>
#include <SoundIo/SoundIo.h>
#include <FS/Bundle.h>
#include <Fonts/Fonts.h>
#include <Shaders/Shaders.h>
#include <Debug/Instrumentation.h>
#include <Ty/ArenaAllocator.h>
#include <MS/Plugin.h>
#include <MS/Project.h>
#include <MS/PluginManager.h>
#include <UI/Application.h>
#include <UI/Window.h>
#include <Render/Render.h>

template <typename T>
void swap(T* a, T* b)
{
    T c = *a;
    T d = *b;
    *a = d;
    *b = c;
}

ErrorOr<int> Main::main(int argc, c_string *argv)
{
    auto bundle = FS::Bundle()
        .add_pack(Fonts())
        .add_pack(Shaders());

    dbgln("resources:"sv);
    for (auto resource : bundle.resources()) {
        dbgln("  "sv, resource.resolved_path());
    }

    auto argument_parser = CLI::ArgumentParser();

    auto root_directory = "."sv;
    TRY(argument_parser.add_option("--root", "-r", "directory", "root directory", [&](c_string arg) {
        root_directory = StringView::from_c_string(arg);
    }));

    auto plugin_paths = Vector<c_string>();
    TRY(argument_parser.add_option("--plugin", "-p", "plugin", "open plugin", [&](c_string arg) {
        MUST(plugin_paths.append(arg));
    }));

    auto hot_reload = ""sv;
    TRY(argument_parser.add_option("--hot-reload", "-h", "shared-library", "hot reload", [&](c_string arg) {
        hot_reload = StringView::from_c_string(arg);
    }));

    TRY(argument_parser.add_flag("--call-graph", "-c", "show function call graph", [&]() {
        Debug::Instrumentation::enabled = true;
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    auto* app = ui_application_create(0);
    auto* window = ui_window_create(app, {
        .parent = nullptr,
        .title = "MusicStudio",
        .x = 0,
        .y = 0,
        .width = 900,
        .height = 600,
    });

    auto arena = TRY(ArenaAllocator::create(1024ULL * 1024ULL));
    auto project = MS::Project();
    auto manager = TRY(MS::PluginManager::create(&arena)).init(&project);
    auto plugins = TRY(arena.alloc<Id<MS::Plugin>>(1024));

    usize plugin_count = 0;
    for (usize i = 0; i < plugin_paths.size(); i++) {
        plugin_count += 1;
        plugins[i] = TRY(manager.instantiate(plugin_paths[i]));
    }
    (void)plugin_count;
    for (usize i = 0; i < plugin_count; i++) {
        manager.plugins[i].init();
    }

    auto out_frames = TRY(arena.alloc<f32>(project.sample_rate * project.channels)).zero();
    auto in_frames = TRY(arena.alloc<f32>(project.sample_rate * project.channels)).zero();
    usize even = 0;
    for (usize i = 0; i < plugin_count; i++) {
        even = (even + 1) & 1;
        manager.plugins[i].process_f32(out_frames.data(), in_frames.data(), project.sample_rate);
        swap(&out_frames, &in_frames);
    }
    swap(&out_frames, &in_frames);
    dbgln(out_frames[0]);

    auto* render = render_create(&bundle, &arena);
    if (!render) {
        return Error::from_string_literal("could not create renderer");
    }
    Defer destroy_render = [&] {
        render_destroy(render);
    };

    Vec2f zero  = { 0.0, 0.0 };
    Vec4f cyan = { .r = 0.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
    Vec4f magenta = { .r = 1.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f };
    Vec4f yellow = { .r = 1.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };
    Vec4f white = { .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
    while (!ui_window_should_close(window)) {
        ui_application_poll_events(app);
        ui_window_gl_make_current_context(window);

        i32 width = 0;
        i32 height = 0;
        ui_window_size(window, &width, &height);
        render_set_resolution(render, vec2f(width, height) * 2.0);

        render_use_simple(render);

        render_transact(render, 6);
            render_vertex(render, { 0.0, 0.0 }, cyan, zero);
            render_vertex(render, { 1.0, 0.0 }, yellow, zero);
            render_vertex(render, { 1.0, 1.0 }, magenta, zero);

            render_vertex(render, { 0.0, 0.0 }, cyan, zero);
            render_vertex(render, { 0.0, 1.0 }, white, zero);
            render_vertex(render, { 1.0, 1.0 }, magenta, zero);
        render_flush(render);

        ui_window_gl_flush(window);
    }

    return 0;
}

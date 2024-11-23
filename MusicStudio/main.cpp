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

static Vec2f zero  = { 0.0, 0.0 };
static Vec4f cyan = { .r = 0.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
static Vec4f magenta = { .r = 1.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f };
static Vec4f yellow = { .r = 1.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };
static Vec4f white = { .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
static Vec4f red = { .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };

static void render_frame(UIWindow* window, Render* render);

ErrorOr<int> Main::main(int argc, c_string *argv)
{
    auto bundle = FS::Bundle()
        .add_pack(Fonts())
        .add_pack(Shaders());

    dprintln("resources:");
    for (auto resource : bundle.resources()) {
        dprintln("  {}", resource.resolved_path());
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
    dprintln("{}", out_frames[0]);

    auto* render = render_create(&bundle, &arena);
    if (!render) {
        return Error::from_string_literal("could not create renderer");
    }
    Defer destroy_render = [&] {
        render_destroy(render);
    };

    ui_window_set_resize_callback(window, render, [](UIWindow* window, void* user) {
        auto* render = (Render*)user;
        ui_window_gl_make_current_context(window);

        i32 width = 0;
        i32 height = 1;
        ui_window_size(window, &width, &height);
        render_set_resolution(render, vec2f(width, height) * 2.0);

        render_frame(window, render);

        render_flush(render);
        ui_window_gl_flush(window);
    });

    {
        i32 width = 0;
        i32 height = 1;
        ui_window_size(window, &width, &height);
        render_set_resolution(render, vec2f(width, height) * 2.0);
    }

    while (!ui_window_should_close(window)) {
        ui_application_poll_events(app);
        ui_window_gl_make_current_context(window);

        i32 mouse_x = 0;
        i32 mouse_y = 0;
        ui_window_mouse_pos(window, &mouse_x, &mouse_y);
        render_set_mouse_position(render, vec2f(mouse_x, mouse_y));

        render_frame(window, render);
        render_flush(render);
        ui_window_gl_flush(window);
    }

    return 0;
}

static void render_frame(UIWindow* window, Render* render)
{
    render_clear(render, vec4f(0, 0, 0, 1));

    if (ui_window_is_fullscreen(window)) {
        render_quad(render,
            vec2f(0.0f, 0.0f), cyan,    zero,
            vec2f(1.0f, 0.0f), yellow,  zero,
            vec2f(0.0f, 1.0f), white,   zero,
            vec2f(1.0f, 1.0f), magenta, zero
        );
    } else {
        f32 titlebar_height = 0;
        i32 height = 1;
        ui_window_size(window, 0, &height);
        titlebar_height = 28.0f / height;

        Vec4f color = cyan / 2;
        i32 mouse_y = 0;
        ui_window_mouse_pos(window, 0, &mouse_y);
        if (ui_window_mouse_state(window).left_down >= 2) {
            color = red;
        }

        render_quad(render,
            vec2f(0.0f, titlebar_height), cyan,    zero,
            vec2f(1.0f, titlebar_height), yellow,  zero,
            vec2f(0.0f, 1.0f), white,   zero,
            vec2f(1.0f, 1.0f), magenta, zero
        );
        render_quad(render,
            vec2f(0.0, 0.0),             color, zero,
            vec2f(1.0, 0.0),             color, zero,
            vec2f(0.0, titlebar_height), color, zero,
            vec2f(1.0, titlebar_height), color, zero
        );
    }
}

}

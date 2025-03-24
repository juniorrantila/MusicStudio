#include <CLI/ArgumentParser.h>
#include <Ty/Defer.h>
#include <Ty/ErrorOr.h>
#include <UI/Application.h>
#include <UI/Window.h>
#include <Main/Main.h>
#include <Ty/StringSlice.h>
#include <Render/Render.h>
#include <Core/Time.h>
#include <Ty2/PageAllocator.h>
#include <Ty2/Arena.h>
#include <Ty2/FileLogger.h>
#include <sys/time.h>

#include <stdio.h>

struct Context {
    Render* render;
};

static void ui_frame(Context*);

ErrorOr<int> Main::main(int argc, char const* argv[])
{
    auto arena = arena_create(page_allocator());
    auto volume_arena_instance = arena_create(&arena.allocator);
    auto* volume_arena = &volume_arena_instance.allocator;
    auto render_arena_instance = arena_create(&arena.allocator);
    auto* render_arena = &render_arena_instance.allocator;
    auto log_arena_instance = arena_create(&arena.allocator);
    auto* log_arena = &log_arena_instance.allocator;
    auto file_logger = make_file_logger(log_arena, stderr);
    auto* log = &file_logger.logger;

    FSVolume* volume = fs_volume_create(volume_arena);
    if (!volume) log->fatal("could not create file volume");

    auto argument_parser = CLI::ArgumentParser();

    StringSlice vertex_shader_path = "Shaders/simple.vert"s;
    TRY(argument_parser.add_option("--vertex"sv, "-v", "path", "vertex shader path (default: Shaders/simple.vert)", [&](c_string arg) {
        vertex_shader_path = string_slice_from_c_string(arg);
    }));

    StringSlice fragment_shader_path = "Shaders/color.frag"s;
    TRY(argument_parser.add_option("--fragment"sv, "-f", "path", "fragment shader path (default: Shaders/color.frag)", [&](c_string arg) {
        fragment_shader_path = string_slice_from_c_string(arg);
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    FSFile vertex_file;
    if (!fs_system_open(volume_arena, vertex_shader_path, &vertex_file)) {
        log->fatal("could not open '%.*s'", (int)vertex_shader_path.count, vertex_shader_path.items);
    }
    vertex_file.virtual_path = "Shaders/simple.vert"s;
    if (!fs_volume_mount(volume, vertex_file, 0)) {
        log->fatal("could not mount '%.*s'", (int)vertex_shader_path.count, vertex_shader_path.items);
    }

    FSFile fragment_file;
    if (!fs_system_open(volume_arena, fragment_shader_path, &fragment_file)) {
        log->fatal("could not open '%.*s'", (int)fragment_shader_path.count, fragment_shader_path.items);
    }
    fragment_file.virtual_path = "Shaders/color.frag"s;
    if (!fs_volume_mount(volume, fragment_file, 0)) {
        log->fatal("could not mount '%.*s'", (int)fragment_shader_path.count, fragment_shader_path.items);
    }

    auto* app = ui_application_create(0);
    defer [&] {
        ui_application_destroy(app);
    };

    auto* window = ui_window_create(app, {
        .parent = nullptr,
        .title = "shadertoy",
        .x = 0,
        .y = 0,
        .width = 800,
        .height = 600,
    });
    defer [&] {
        ui_window_destroy(window);
    };
    ui_window_autosave(window, "com.music-studio.shadertoy");

    auto* render = render_create(volume, render_arena, log);
    if (!render) log->fatal("could not create render");

    auto context = Context {
        .render = render,
    };

    ui_window_set_resize_callback(window, &context, [](UIWindow* window, void* user) {
        auto* context = (Context*)user;
        auto size = ui_window_size(window);
        auto ratio = ui_window_pixel_ratio(window);
        render_set_resolution(context->render, size * ratio);

        ui_window_gl_make_current_context(window);
        ui_frame(context);
        render_flush(context->render);
        ui_window_gl_flush(window);
    });

    ui_window_gl_make_current_context(window);
    auto size = ui_window_size(window);
    auto ratio = ui_window_pixel_ratio(window);
    render_set_resolution(render, size * ratio);

    auto begin = Core::time();
    while (!ui_window_should_close(window)) {
        ui_application_poll_events(app);

        struct timespec time {};
        fs_volume_poll_events(volume, &time);
        render_update(render);

        if (render_needs_reload(render)) {
            if (render_reload(render)) log->info("reloaded shaders");
            else log->warning("could not reload shaders");
        }

        render_set_time(render, (f32)(Core::time() - begin));
        ui_window_gl_make_current_context(window);
        ui_frame(&context);
        render_flush(render);
        ui_window_gl_flush(window);
    }

    return 0;
}

static void ui_frame(Context* ctx)
{
    auto resolution = render_resolution(ctx->render);
    render_clear(ctx->render, vec4f(0, 0, 0, 1));
    render_quad(ctx->render,
        vec2f(0, 0), vec4f(1, 0, 0, 1), vec2f(0.0,          0.0),
        vec2f(1, 0), vec4f(0, 1, 0, 1), vec2f(resolution.x, 0.0),
        vec2f(0, 1), vec4f(0, 0, 1, 1), vec2f(0.0,          resolution.y),
        vec2f(1, 1), vec4f(1, 1, 1, 1), vec2f(resolution.x, resolution.y)
    );
}

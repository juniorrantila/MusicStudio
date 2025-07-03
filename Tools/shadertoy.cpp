#include <CLI/ArgumentParser.h>
#include <Ty/Defer.h>
#include <Ty/ErrorOr.h>
#include <UI/Application.h>
#include <UI/Window.h>
#include <Main/Main.h>
#include <Ty/StringSlice.h>
#include <GL/Renderer.h>
#include <Core/Time.h>
#include <Ty2/PageAllocator.h>
#include <Ty2/FixedArena.h>
#include <Ty2/FileLogger.h>
#include <FS/FSVolume.h>
#include <sys/time.h>

#include <stdio.h>

struct Context {
    GLRenderer* render;
    UIWindow* window;
    FSVolume* volume;
    Logger* log;
};

static void ui_frame(Context*);

ErrorOr<int> Main::main(int argc, char const* argv[])
{
    auto file_logger = file_logger_init(stderr);
    auto* log = &file_logger.logger;

    FSVolume volume = (FSVolume){};
    fs_volume_init(&volume);

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
    if (!fs_system_open(page_allocator(), vertex_shader_path, &vertex_file)) {
        log->fatal("could not open '%.*s'", (int)vertex_shader_path.count, vertex_shader_path.items);
    }
    vertex_file.virtual_path = "Shaders/simple.vert"s;
    if (!fs_volume_mount(&volume, vertex_file, 0)) {
        log->fatal("could not mount '%.*s'", (int)vertex_shader_path.count, vertex_shader_path.items);
    }

    FSFile fragment_file;
    if (!fs_system_open(page_allocator(), fragment_shader_path, &fragment_file)) {
        log->fatal("could not open '%.*s'", (int)fragment_shader_path.count, fragment_shader_path.items);
    }
    fragment_file.virtual_path = "Shaders/color.frag"s;
    if (!fs_volume_mount(&volume, fragment_file, 0)) {
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

    GLRenderer renderer = (GLRenderer){};
    auto context = Context {
        .render = &renderer,
        .window = window,
        .volume = &volume,
        .log = log,
    };

    ui_window_set_resize_callback(window, &context, [](UIWindow* window, void* user) {
        auto* context = (Context*)user;
        auto size = ui_window_size(window);
        auto ratio = ui_window_pixel_ratio(window);
        context->render->uniform2f(context->render->uniform("resolution"), (v2){ size.x * ratio, size.y * ratio });

        ui_window_gl_make_current_context(window);
        ui_frame(context);

        context->render->flush();
        ui_window_gl_flush(window);
    });

    ui_window_gl_make_current_context(window);
    auto size = ui_window_size(window);
    auto ratio = ui_window_pixel_ratio(window);
    renderer.uniform2f(renderer.uniform("resolution"), (v2){ size.x * ratio, size.y * ratio });

    while (!ui_window_should_close(window)) {
        ui_application_poll_events(app);

        struct timespec time {};
        auto events = fs_volume_poll_events(&volume, &time);
        for (usize i = 0; i < events.count; i++) {
            if (events.items[i].kind == FSEventKind_Modify)
                fs_file_reload(events.items[i].file);
            if (events.items[i].kind == FSEventKind_Create)
                fs_file_reload(events.items[i].file);
        }

        ui_window_gl_make_current_context(window);
        renderer.uniform1f(renderer.uniform("time"), (f32)core_time_since_start());

        ui_frame(&context);

        renderer.flush();
        ui_window_gl_flush(window);
    }

    return 0;
}

static GLVertex vertex(v2 position, v4 color, v2 uv0 = 0, v2 uv1 = 0, v4 bits = 0) {
    return (GLVertex) {
            .color = color,
            .bits = bits,
            .position = position,
            .uv0 = uv0,
            .uv1 = uv1,
    };
}

static void ui_frame(Context* ctx)
{
    auto size = ui_window_size(ctx->window);
    auto ratio = ui_window_pixel_ratio(ctx->window);
    v2 resolution = (v2){size.x * ratio, size.y * ratio};

    ctx->render->clear((v4){0, 0, 0, 1});

    FileID vert;
    if (!fs_volume_find(ctx->volume, "Shaders/simple.vert"s, &vert))
        return;

    FileID frag;
    if (!fs_volume_find(ctx->volume, "Shaders/color.frag"s, &frag))
        return;

    ctx->render->push_quad(ctx->render->shader(ctx->log, GLShaderSource{
        .vert = fs_content(fs_volume_use(ctx->volume, vert)),
        .frag = fs_content(fs_volume_use(ctx->volume, frag)),
    }), GLQuad{
        vertex((v2){0, 0}, (v4){1, 0, 0, 1}, (v2){0,            0}),
        vertex((v2){1, 0}, (v4){0, 1, 0, 1}, (v2){resolution.x, 0}),
        vertex((v2){0, 1}, (v4){0, 0, 1, 1}, (v2){0,            resolution.y}),
        vertex((v2){1, 1}, (v4){1, 1, 1, 1}, (v2){resolution.x, resolution.y}),
    });
}

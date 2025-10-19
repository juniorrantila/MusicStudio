#include <Basic/Context.h>
#include <Basic/FileLogger.h>
#include <Basic/FixedArena.h>
#include <Basic/PageAllocator.h>
#include <Basic/StringSlice.h>

#include <LibCLI/ArgumentParser.h>
#include <LibCore/FSVolume.h>
#include <LibCore/Time.h>
#include <LibGL/Renderer.h>
#include <LibMain/Main.h>
#include <LibTy/Defer.h>
#include <LibTy/ErrorOr.h>
#include <LibUI/Application.h>
#include <LibUI/Window.h>

#include <sys/time.h>
#include <stdio.h>

struct State {
    GLRenderer* render;
    UIWindow* window;
    FSVolume* volume;
};

static void ui_frame(State*);

ErrorOr<int> Main::main(int argc, char const* argv[])
{
    init_default_context("shadertoy");

    FSVolume volume = (FSVolume){};
    fs_volume_init(&volume);

    auto argument_parser = CLI::ArgumentParser();

    StringSlice vertex_shader_path = "Shaders/simple.vert"s;
    TRY(argument_parser.add_option("--vertex"sv, "-v", "path", "vertex shader path (default: Shaders/simple.vert)", [&](c_string arg) {
        vertex_shader_path = sv_from_c_string(arg);
    }));

    StringSlice fragment_shader_path = "Shaders/color.frag"s;
    TRY(argument_parser.add_option("--fragment"sv, "-f", "path", "fragment shader path (default: Shaders/color.frag)", [&](c_string arg) {
        fragment_shader_path = sv_from_c_string(arg);
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }

    FSFile vertex_file;
    if (!fs_system_open(page_allocator(), vertex_shader_path, &vertex_file)) {
        fatalf("could not open '%.*s'", (int)vertex_shader_path.count, vertex_shader_path.items);
    }
    vertex_file.virtual_path = "Shaders/simple.vert"s;
    if (!fs_volume_mount(&volume, vertex_file, 0)) {
        fatalf("could not mount '%.*s'", (int)vertex_shader_path.count, vertex_shader_path.items);
    }

    FSFile fragment_file;
    if (!fs_system_open(page_allocator(), fragment_shader_path, &fragment_file)) {
        fatalf("could not open '%.*s'", (int)fragment_shader_path.count, fragment_shader_path.items);
    }
    fragment_file.virtual_path = "Shaders/color.frag"s;
    if (!fs_volume_mount(&volume, fragment_file, 0)) {
        fatalf("could not mount '%.*s'", (int)fragment_shader_path.count, fragment_shader_path.items);
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
    auto state = State {
        .render = &renderer,
        .window = window,
        .volume = &volume,
    };

    ui_window_set_resize_callback(window, &state, [](UIWindow* window, void* user) {
        auto* state = (State*)user;
        auto size = ui_window_size(window);
        auto ratio = ui_window_pixel_ratio(window);
        state->render->uniform2f(state->render->uniform("resolution"), (v2){ size.x * ratio, size.y * ratio });

        ui_window_gl_make_current_context(window);
        ui_frame(state);

        state->render->flush();
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
            if (events.kind[i] == FSEventKind_Modify)
                fs_file_reload(fs_volume_use_ref(&volume, events.file[i]));
            if (events.kind[i] == FSEventKind_Create)
                fs_file_reload(fs_volume_use_ref(&volume, events.file[i]));
        }

        ui_window_gl_make_current_context(window);
        renderer.uniform1f(renderer.uniform("time"), (f32)core_time_since_unspecified_epoch());

        ui_frame(&state);

        renderer.flush();
        ui_window_gl_flush(window);
    }

    return 0;
}

static GLVertex vertex(v2 position, v4 color, v2 uv0 = v2_zero, v2 uv1 = v2_zero, v4 bits = v4_zero) {
    return (GLVertex) {
            .color = color,
            .bits = bits,
            .position = position,
            .uv0 = uv0,
            .uv1 = uv1,
    };
}

static void ui_frame(State* ctx)
{
    auto size = ui_window_size(ctx->window);
    auto ratio = ui_window_pixel_ratio(ctx->window);
    v2 resolution = (v2){size.x * ratio, size.y * ratio};

    ctx->render->clear(v4f(0, 0, 0, 1));

    FileID vert;
    if (!fs_volume_find(ctx->volume, "Shaders/simple.vert"s, &vert))
        return;

    FileID frag;
    if (!fs_volume_find(ctx->volume, "Shaders/color.frag"s, &frag))
        return;

    ctx->render->push_quad(ctx->render->shader(GLShaderSource{
        .vert = fs_content(fs_volume_use(ctx->volume, vert)),
        .frag = fs_content(fs_volume_use(ctx->volume, frag)),
    }), GLQuad{
        vertex(v2f(0, 0), v4f(1, 0, 0, 1), v2f(0,            0)),
        vertex(v2f(1, 0), v4f(0, 1, 0, 1), v2f(resolution.x, 0)),
        vertex(v2f(0, 1), v4f(0, 0, 1, 1), v2f(0,            resolution.y)),
        vertex(v2f(1, 1), v4f(1, 1, 1, 1), v2f(resolution.x, resolution.y)),
    });
}

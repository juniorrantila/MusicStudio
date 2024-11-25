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
#include <UI/UI.h>

template <typename T>
void swap(T* a, T* b)
{
    T c = *a;
    T d = *b;
    *a = d;
    *b = c;
}

Vec2f zero  = { 0.0, 0.0 };
Vec4f cyan = { .r = 0.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
Vec4f magenta = { .r = 1.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f };
Vec4f yellow = { .r = 1.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };
Vec4f white = { .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
Vec4f red = { .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };
Vec4f green = { .r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };
Vec4f blue = { .r = 0.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f };
Vec4f border_color = hex_to_vec4f(0x2C3337FF);
Vec4f outline_color = hex_to_vec4f(0x4B5255FF);
Vec4f background_color = hex_to_vec4f(0x181F23FF);
Vec4f button_color = hex_to_vec4f(0x99A89FFF);
Vec4f gray_background = hex_to_vec4f(0x646A71FF);
Vec4f toolbar_color = hex_to_vec4f(0x5B6265FF);

static void render_frame(UI* ui);

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
    auto ui = ui_create(window, render);

    ui_window_set_resize_callback(window, &ui, [](UIWindow*, void* user) {
        auto* ui = (UI*)user;
        ui_begin_frame(ui);
        render_frame(ui);
        ui_end_frame(ui);
    });

    while (!ui_window_should_close(window)) {
        ui_application_poll_events(app);

        ui_begin_frame(&ui);
        render_frame(&ui);
        ui_end_frame(&ui);
    }

    return 0;
}

static void file_browser(UI* ui);
static void toolbar(UI* ui);
static void status_bar(UI* ui);

static void render_frame(UI* ui)
{
    render_clear(ui->render, vec4f(0, 0, 0, 1));
    auto window_size = ui_window_size(ui->window);

    Vec4f color = toolbar_color;
    if (ui_window_mouse_state(ui->window).left_down >= 2) {
        color = red;
    }

    auto point = ui_current_point(ui);
    ui_rect(ui, window_size, gray_background);
    ui_move_point(ui, point);

    auto titlebar_height = 28.0f;
    if (!ui_window_is_fullscreen(ui->window)) {
        ui_spacer(ui, vec2f(0, titlebar_height));
    }

    toolbar(ui);
    file_browser(ui);
    status_bar(ui);

    if (!ui_window_is_fullscreen(ui->window)) {
        ui_move_point(ui, point);
        ui_rect(ui, vec2f(window_size.x, titlebar_height), color);
    }
}

static void toolbar(UI* ui)
{
    auto window_size = ui_window_size(ui->window);
    ui_rect(ui, vec2f(window_size.x, 28), toolbar_color);
    ui_rect(ui, vec2f(window_size.x, 2), outline_color);
}

static void status_bar(UI* ui)
{
    auto window_size = ui_window_size(ui->window);
    ui_move_point(ui, vec2f(0, window_size.y - 30));
    ui_rect(ui, vec2f(window_size.x, 2), outline_color);
    ui_rect(ui, vec2f(window_size.x, 28), toolbar_color);
}

static void file_browser(UI* ui)
{
    auto window_size = ui_window_size(ui->window);

    auto pos = ui_current_point(ui);
    ui_spacer(ui, vec2f(200, -2));
    ui_rect(ui, vec2f(2, window_size.y), outline_color);
    ui_move_point(ui, pos);
    ui_rect(ui, vec2f(200, window_size.y), background_color);
    ui_move_point(ui, pos);
    ui_rect(ui, vec2f(8, window_size.y), border_color);
    ui_move_point(ui, pos);

    ui_spacer(ui, vec2f(12, 4));
    if (ui_button(ui, "Button 1", button_color)) {
        dprintln("Button 1");
    }
    ui_spacer(ui, vec2f(0, 8));
    if (ui_button(ui, "Button 2", button_color)) {
        dprintln("Button 2");
    }
    ui_spacer(ui, vec2f(0, 8));
    if (ui_button(ui, "Button 3", button_color)) {
        dprintln("Button 3");
    }
}

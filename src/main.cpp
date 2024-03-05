#include "./Common.h"
#include "./FileBrowser.h"
#include "./EventLoop.h"
#include "./StatusBar.h"
#include "./Toolbar.h"
#include "./Style.h"

#include <Bundle/Bundle.h>
#include <CLI/ArgumentParser.h>
#include <Core/Print.h>
#include <Main/Main.h>
#include <Rexim/LA.h>
#include <Ty/Forward.h>
#include <UI/Application.h>
#include <UI/FreeGlyph.h>
#include <UI/KeyCode.h>
#include <UI/SimpleRenderer.h>
#include <UI/UI.h>
#include <UI/Window.h>
#include <Vst/Rectangle.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <fcntl.h>

#include <MS/Plugin.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdarg.h>

struct OpenPluginEvent {
    mutable StringBuffer file_path_buf;

    c_string file_path() const
    {
        if (file_path_buf.data()[file_path_buf.size() - 1] != '\0') {
            MUST(file_path_buf.write("\0"sv));
        }
        return file_path_buf.data();
    }
};

struct MainWidget : Widget {
    Toolbar& toolbar;
    FileBrowser& file_browser;
    StatusBar& status_bar;

    MainWidget(Toolbar& toolbar, FileBrowser& file_browser, StatusBar& status_bar)
        : toolbar(toolbar)
        , file_browser(file_browser)
        , status_bar(status_bar)
    {
    }

    void render(UI::UI& ui, EventLoop& event_loop, Vec4f box) override;
};

ErrorOr<int> Main::main(int argc, c_string argv[])
{
    auto event_loop = EventLoop();
    auto argument_parser = CLI::ArgumentParser();

    auto root_directory = "."sv;
    TRY(argument_parser.add_option("--root", "-r", "directory", "root directory", [&](c_string arg) {
        root_directory = StringView::from_c_string(arg);
    }));

    auto plugin_paths = Vector<StringBuffer>();
    TRY(argument_parser.add_option("--plugin", "-p", "plugin", "open plugin", [&](c_string arg) {
        auto path = StringView::from_c_string(arg);
        MUST(plugin_paths.append(MUST(StringBuffer::create_fill(path, "\0"sv))));
    }));

    if (auto result = argument_parser.run(argc, argv); result.is_error()) {
        TRY(result.error().show());
        return 1;
    }
    for (auto& plugin : plugin_paths) {
        MUST(event_loop.dispatch_event(OpenPluginEvent {
            .file_path_buf = move(plugin),
        }));
    }

    auto toolbar = Toolbar();
    auto file_browser = TRY(FileBrowser::create(root_directory));
    auto& status_bar = StatusBar::the();

    dbgln("resources:"sv);
    for (auto resource : Bundle::the().resources()) {
        dbgln("  "sv, resource.resolved_path());
    }

    auto application = TRY(UI::Application::create("MusicStudio"sv, 0, 0, 800, 600));

    auto sr = TRY(UI::SimpleRenderer::create());
    auto titlebar_size = 32.0f; // FIXME: This should not be needed
    sr.set_resolution(vec2f(application.width(), application.height() + titlebar_size));
    sr.set_camera_pos(vec2f(application.width(), application.height() + titlebar_size) / 2.0f);
    auto ui = UI::UI(&sr);

    // TODO: users should be able to customize the font
    // const char *const font_file_path = "./Fonts/VictorMono-Regular.ttf";
    // const char *const font_file_path = "./Fonts/iosevka-regular.ttf";
    auto font_path = "./Fonts/OxaniumLight/Oxanium-Light.ttf"sv;
    auto font = TRY(Bundle::the().resource_with_path(font_path).or_throw([&]{
        return Error::from_string_literal("could not find font");
    }));

    TRY(ui.load_font(font.bytes(), vec2f(0.0f, 24.0f)));

    ui.set_title_bar_height(32.0f);

    application.on_window_resize = [&](f32 width, f32 height) {
        sr.set_resolution(vec2f(width, height));
        sr.set_camera_pos(vec2f(width, height) / 2.0f);
    };

    application.on_mouse_move = [&](f32 x, f32 y) {
        ui.set_mouse_pos(x, y);
    };

    application.on_mouse_down = [&] {
        ui.set_mouse_down(true);
    };

    application.on_mouse_up = [&] {
        ui.set_mouse_down(false);
    };

    application.on_scroll = [&](f32 x, f32 y) {
        ui.set_scroll_x(x);
        ui.set_scroll_y(y);
    };

    auto plugins = Vector<MS::Plugin>();

    auto handle_events = [&] {
        for (auto event : event_loop.events()) {
            event.match(

                On<OpenPluginEvent>([&](OpenPluginEvent const& plugin_event) {
                    auto file_path = plugin_event.file_path();
                    auto result = MS::Plugin::create_from(file_path);
                    if (result.is_error()) {
                        flash_error("Could not load plugin '%s'", file_path);
                        return;
                    }
                    auto plugin_id = MUST(plugins.append(result.release_value()));
                    auto& plugin = plugins[plugin_id];
                    if (plugin.has_editor()) {
                        auto rect = plugin.editor_rectangle().or_else(Vst::Rectangle{
                            .y = 0,
                            .x = 0,
                            .height = 600,
                            .width = 800,
                        });

                        auto name = plugin.name().or_else(plugin_event.file_path_buf.view());
                        auto x = rect.x;
                        auto y = rect.y;
                        auto width = rect.width;
                        auto height = rect.height;
                        auto result = UI::Window::create(name, x, y, width, height);
                        if (result.is_error()) {
                            auto message = result.error().message();
                            flash_error("%.*s: Could not open plugin editor: %.*s",
                                name.size(), name.data(), message.size(), message.data());
                            return;
                        }
                        auto plugin_window = result.release_value();
                        application.add_child_window(plugin_window);
                        if (!plugin.open_editor(plugin_window->native_handle())) {
                            flash_error("%.*s: Could not open editor for plugin\n",
                                name.size(), name.data());
                            return;
                        }
                        plugin.on_editor_resize = [plugin_window](i32 x, i32 y) {
                            plugin_window->resize(x, y);
                        };
                    }
                }),

                On<ChangePathEvent>([&](ChangePathEvent const& path) {
                    auto file_path = path.file_path();

                    File_Type ft;
                    Errno err = type_of_file(file_path, &ft);
                    if (err != 0) {
                        flash_error("Could not determine type of file %s: %s", file_path, strerror(err));
                        return;
                    }
                    switch (ft) {
                    case FT_DIRECTORY: {
                        file_browser.change_dir().or_else([=](Error error){
                            auto message = error.message();
                            flash_error("Could not change directory to %s: %.*s", file_path, message.size(), message.data());
                        });
                    }
                    break;

                    case FT_REGULAR: {
                        MUST(event_loop.dispatch_event(OpenPluginEvent {
                            .file_path_buf = move(path.file_path_buf),
                        }));
                    }
                    break;

                    case FT_OTHER: {
                        flash_error("%s is neither a regular file nor a directory. We can't open it.", file_path);
                    }
                    break;

                    default:
                        UNREACHABLE("unknown File_Type");
                    }

                })
            );
        }
    };

    auto main_windget = MainWidget(toolbar, file_browser, status_bar);
    application.on_update = [&] {
        handle_events();

        ui.begin_frame();
        ui.clear(Style::the().background_color());

        main_windget.render(ui, event_loop, vec4fv(
            vec2fs(0.0f),
            sr.resolution()
        ));

        ui.end_frame();
    };

    application.run();
    return 0;
}

void MainWidget::render(UI::UI& ui, EventLoop& event_loop, Vec4f box)
{
    Vec2f toolbar_size = vec2f(box.width, 48.0f);
    toolbar.render(ui, event_loop, vec4fv(
        vec2f(0.0f, box.height - toolbar_size.y),
        toolbar_size
    ));
    box.height -= toolbar_size.y;

    file_browser.render(ui, event_loop, vec4fv(
        vec2fs(0.0f),
        vec2f(200.0f, box.height)
    ));

    auto status_bar_size = vec2f(box.width, 32.0f);
    status_bar.render(ui, event_loop, vec4fv(
       vec2fs(0.0f),
       status_bar_size
    ));
}

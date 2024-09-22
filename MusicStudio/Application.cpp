#include "./Application.h"
#include "./StatusBar.h"
#include "./Style.h"

#include "./PathEvent.h"

#include <UI/Application.h>
#include <FS/Bundle.h>
#include <Ty/Try.h>
#include <Ty/Move.h>
#include <UI/Window.h>

Application::Application(UI::Application&& application, UI::UI&& ui, FileBrowser&& file_browser)
    : m_application(move(application))
    , m_ui(move(ui))
    , m_file_browser(move(file_browser))
{
}

ErrorOr<Application> Application::create(FS::Bundle& bundle)
{
    auto application = TRY(UI::Application::create("MusicStudio"sv, 0, 0, 800, 600));
    auto sr = TRY(UI::SimpleRenderer::create(bundle));
    auto titlebar_size = 32.0f; // FIXME: This should not be needed
    sr.set_resolution(vec2f(application.width(), application.height() + titlebar_size));
    sr.set_camera_pos(vec2f(application.width(), application.height() + titlebar_size) / 2.0f);
    auto ui = UI::UI(move(sr));
    ui.set_title_bar_height(32.0f);

    // TODO: users should be able to customize the font
    // const char *const font_file_path = "./Fonts/VictorMono-Regular.ttf";
    // const char *const font_file_path = "./Fonts/iosevka-regular.ttf";
    auto font_path = "./Fonts/OxaniumLight/Oxanium-Light.ttf"sv;
    auto font = TRY(bundle.open(font_path).or_throw([&]{
        return Error::from_string_literal("could not find font");
    }));

    TRY(ui.load_font(font.bytes(), vec2f(0.0f, 24.0f)));

    return Application(
        move(application),
        move(ui),
        TRY(FileBrowser::create("."))
    );
}

void Application::run()
{
    MUST(m_application.window_size.add_observer([this](Vec2f size) {
        m_ui.set_resolution(size);
    }));

    MUST(m_application.mouse_pos.add_observer([this](Vec2f pos) {
        m_ui.set_mouse_pos(pos.x, pos.y);
    }));

    MUST(m_application.is_mouse_left_down.add_observer([this](bool down) {
        m_ui.set_mouse_down(down);
    }));

    MUST(m_application.scroll.add_observer([this](Vec2f scroll) {
        m_ui.set_scroll_x(scroll.x);
        m_ui.set_scroll_y(scroll.y);
    }));

    m_application.on_update = [this]() {
        handle_events();

        m_ui.begin_frame();
        m_ui.clear(Style::the().background_color());

        render(vec4fv(
            vec2fs(0.0f),
            m_ui.resolution()
        ));

        m_ui.end_frame();
    };

    MUST(m_current_path.add_observer([this](StringBuffer const& path) {
        MUST(change_path(path.view()));
    }));

    m_application.run();
}

ErrorOr<void> Application::open_plugin(StringView path)
{
    auto buf = TRY(StringBuffer::create_fill(path, "\0"sv));
    auto result = MS::Plugin::create_from(buf.data());
    if (result.is_error()) {
        flash_error("Could not load plugin '%s'", buf.data());
        return {};
    }
    auto plugin_id = MUST(m_plugins.append(result.release_value()));
    auto& plugin = m_plugins[plugin_id];
    if (plugin.has_editor()) {
        auto rect = plugin.editor_rectangle().or_else(Vst::Rectangle{
            .y = 0,
            .x = 0,
            .height = 600,
            .width = 800,
        });

        auto name = plugin.name().or_else(path);
        auto x = rect.x;
        auto y = rect.y;
        auto width = rect.width;
        auto height = rect.height;
        auto result = UI::Window::create(name, x, y, width, height);
        if (result.is_error()) {
            auto message = result.error().message();
            flash_error("%.*s: Could not open plugin editor: %.*s",
                name.size(), name.data(), message.size(), message.data());
            return {};
        }
        auto plugin_window = result.release_value();
        m_application.add_child_window(plugin_window);
        if (!plugin.open_editor(plugin_window->native_handle())) {
            flash_error("%.*s: Could not open editor for plugin\n",
                name.size(), name.data());
            return {};
        }
        plugin.on_editor_resize = [plugin_window](i32 x, i32 y) {
            plugin_window->resize(x, y);
        };
    }

    return {};
}

ErrorOr<void> Application::change_path(StringView path)
{
    auto buf = TRY(StringBuffer::create_fill(path, "\0"sv));
    auto file_path = buf.data();

    File_Type ft;
    Errno err = type_of_file(file_path, &ft);
    if (err != 0) {
        flash_error("Could not determine type of file %s: %s", file_path, strerror(err));
        return {};
    }
    switch (ft) {
    case FT_DIRECTORY: {
        // FIXME: Change this in to something like open_dir.
        m_file_browser.change_dir().or_else([=](Error error){
            auto message = error.message();
            flash_error("Could open directory %s: %.*s", file_path, message.size(), message.data());
        });
    }
    break;

    case FT_REGULAR: {
        TRY(open_plugin(buf.view()));
    }
    break;

    case FT_OTHER: {
        flash_error("%s is neither a regular file nor a directory. We can't open it.", file_path);
    }
    break;

    default:
        UNREACHABLE("unknown File_Type");
    }

    return {};
}

void Application::handle_events()
{
    for (auto event : m_event_loop.events()) {
        event.match(
            On<ChangePathEvent>([this](ChangePathEvent const& event) {
                m_current_path.update([&](StringBuffer& path) {
                    path.clear();
                    MUST(path.write(event.file_path_buf.view()));
                });
            })
        );
    }
}

void Application::render(Vec4f box)
{
    Vec2f toolbar_size = vec2f(box.width, 48.0f);
    m_toolbar.render(m_ui, m_event_loop, vec4fv(
        vec2f(0.0f, box.height - toolbar_size.y),
        toolbar_size
    ));
    box.height -= toolbar_size.y;

    m_file_browser.render(m_ui, m_event_loop, vec4fv(
        vec2fs(0.0f),
        vec2f(200.0f, box.height)
    ));

    auto status_bar_size = vec2f(box.width, 32.0f);
    StatusBar::the().render(m_ui, m_event_loop, vec4fv(
       vec2fs(0.0f),
       status_bar_size
    ));
}

#include "./Common.h"
#include "./FileBrowser.h"

#include <Main/Main.h>
#include <Rexim/LA.h>
#include <UI/Application.h>
#include <UI/FreeGlyph.h>
#include <UI/KeyCode.h>
#include <UI/SimpleRenderer.h>
#include <UI/UI.h>
#include <UI/Window.h>
#include <Vst/Rectangle.h>
#include <Bundle/Bundle.h>
#include <Core/Print.h>

#include <MS/Plugin.h>

#include <ft2build.h>
#include FT_FREETYPE_H

static FileBrowser fb = {};

// TODO: display errors reported via flash_error right in the editor window somehow
#define flash_error(fmt, ...) do { fprintf(stderr, "ERROR: " fmt "\n", ## __VA_ARGS__); } while(0)

ErrorOr<int> Main::main(int argc, c_string argv[])
{
    dbgln("resources:"sv);
    for (auto resource : Bundle::the().resources()) {
        dbgln("  "sv, resource.resolved_path());
    }

    Errno err;

    FT_Library library = {0};

    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        fprintf(stderr, "ERROR: Could not initialize FreeType2 library\n");
        return 1;
    }

    // TODO: users should be able to customize the font
    // const char *const font_file_path = "./Fonts/VictorMono-Regular.ttf";
    // const char *const font_file_path = "./Fonts/iosevka-regular.ttf";
    auto font_path = "./Fonts/OxaniumLight/Oxanium-Light.ttf"sv;
    auto font = TRY(Bundle::the().resource_with_path(font_path).or_throw([&]{
        return Error::from_string_literal("could not find font");
    }));

    FT_Face face;
    error = FT_New_Memory_Face(library, font.data(), font.size(), 0, &face);
    if (error == FT_Err_Unknown_File_Format) {
        fprintf(stderr, "ERROR: `%.*s` has an unknown format\n", font_path.size(), font_path.data());
        return 1;
    } else if (error) {
        fprintf(stderr, "ERROR: Could not load file `%.*s`\n", font_path.size(), font_path.data());
        return 1;
    }

    FT_UInt pixel_size = FREE_GLYPH_FONT_SIZE;
    error = FT_Set_Pixel_Sizes(face, 0, pixel_size);
    if (error) {
        fprintf(stderr, "ERROR: Could not set pixel size to %u\n", pixel_size);
        return 1;
    }

    auto plugin = Optional<MS::Plugin>{};
    if (argc > 1) {
        const char *file_path = argv[1];
        const char *dir_path = ".";

        File_Type file_type;
        err = type_of_file(file_path, &file_type);
        if (err != 0) {
            fprintf(stderr, "ERROR: Could not get type of file %s: %s\n", file_path, strerror(err));
            return 1;
        }
        switch (file_type) {
            case FT_REGULAR:
                if (auto result = MS::Plugin::create_from(file_path); result.is_error()) {
                    fprintf(stderr, "ERROR: Could not load plugin '%s'\n", file_path);
                } else {
                    plugin = result.release_value();
                }
                file_path = ".";
                break;

            case FT_DIRECTORY:
                dir_path = file_path;
                break;

            case FT_OTHER:
                fprintf(stderr, "ERROR: Could not get open file %s: unknown file type\n", file_path);
                return -1;
        }
        err = fb_open_dir(&fb, file_path);
        if (err != 0) {
            fprintf(stderr, "ERROR: Could not read directory '%s': %s\n", dir_path, strerror(err));
            return 1;
        }
    } else {
        err = fb_open_dir(&fb, ".");
        if (err != 0) {
            fprintf(stderr, "ERROR: Could not read directory '.': %s\n", strerror(err));
            return 1;
        }
    }

    auto application = TRY(UI::Application::create("MusicStudio"sv, 0, 0, 800, 600));

    auto sr = TRY(UI::SimpleRenderer::create());
    auto titlebar_size = 32.0f; // FIXME: This should not be needed
    sr.set_resolution(vec2f(application.width(), application.height() + titlebar_size));
    sr.set_camera_pos(vec2f(application.width(), application.height() + titlebar_size) / 2.0f);

    auto atlas = TRY(UI::FreeGlyphAtlas::create(face));
    auto ui = UI::UI(&sr, &atlas);

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

    application.on_update = [&] {
        f32 border_size = 2.0f;
        Vec4f background_color = hex_to_vec4f(0x636A72FF);
        Vec4f file_browser_color = hex_to_vec4f(0x161F24FF);
        Vec4f file_browser_border_color = hex_to_vec4f(0x434C51FF);
        Vec4f file_browser_indent_color = hex_to_vec4f(0x2A3338FF);
        Vec4f toolbar_color = hex_to_vec4f(0x596267FF);
        Vec4f toolbar_border_color = hex_to_vec4f(0x495257FF);
        Vec4f info_bar_color = hex_to_vec4f(0x596267FF);
        Vec4f info_bar_border_color = hex_to_vec4f(0x495257FF);
        Vec4f text_color = hex_to_vec4f(0x95A99FFF);
        Vec4f text_alternate_color = hex_to_vec4f(0x95A99FFF);

        ui.begin_frame();

        ui.clear(background_color);

        Vec2f space = sr.resolution();

        // Toolbar
        {
            Vec2f toolbar_size = vec2f(space.x, 48.0f);
            ui.outline_rect({
                .box = vec4fv(
                    vec2f(0.0f, space.y - toolbar_size.y),
                    toolbar_size
                ),
                .outline_size = border_size,
                .fill_color   = toolbar_color,
                .bottom_color = toolbar_border_color,
            });
            space.y -= toolbar_size.y;
        }

        // File browser
        {
            ui.outline_rect({
                .box = vec4fv(
                    vec2fs(0.0f),
                    vec2f(200.0f, space.y)
                ),
                .outline_size = border_size,
                .fill_color = file_browser_color,
                .right_color = file_browser_border_color,
            });

            // Indent
            f32 indent_size = 8.0f;
            ui.fill_rect(vec4fv(
                    vec2f(0.0f, 0.0f),
                    vec2f(indent_size, space.y)
                ),
                file_browser_indent_color
            );

            for (usize row = 0; row < fb.files.count; ++row) {
                const Vec2f pos = vec2f(indent_size + 2.0f, space.y - ((f32)row + 1) * FREE_GLYPH_FONT_SIZE);
                StringView file_name = StringView::from_c_string(fb.files.items[row].name);

                auto box_size = ui.measure_text(file_name);

                ui.text(pos, file_name, text_color);
                if (fb.files.items[row].type == FT_DIRECTORY) {
                    auto slash_pos = pos + ui.measure_text(file_name);
                    box_size.x += ui.measure_text("/"sv).x;
                    ui.text(slash_pos, "/"sv, text_alternate_color);
                }

                auto box = vec4fv(
                    pos - vec2f(0.0f, 3.0f),
                    vec2f(200.0f - indent_size - 3.0f * 2.0f, FREE_GLYPH_FONT_SIZE)
                );
                auto button = ui.button(box);
                if (button.action) {
                    ui.fill_rect(box, vec4f(1.0f, 0.0f, 0.0f, 0.50));

                    const char *file_path = fb_file_path(&fb);
                    if (file_path) {
                        File_Type ft;
                        Errno err = type_of_file(file_path, &ft);
                        if (err != 0) {
                            flash_error("Could not determine type of file %s: %s", file_path, strerror(err));
                        } else {
                            switch (ft) {
                            case FT_DIRECTORY: {
                                err = fb_change_dir(&fb);
                                if (err != 0) {
                                    flash_error("Could not change directory to %s: %s", file_path, strerror(err));
                                }
                            }
                            break;

                            case FT_REGULAR: {
                                if (auto result = MS::Plugin::create_from(file_path); result.is_error()) {
                                    flash_error("Could not load plugin '%s'", file_path);
                                } else {
                                    auto plugin = result.release_value();
                                    if (plugin.has_editor()) {
                                        auto rect = plugin.editor_rectangle().or_else(Vst::Rectangle{0, 0, 800, 600});
                                        if (auto result = UI::Window::create(plugin.name().or_else(""sv), rect.x, rect.y, rect.width, rect.height); result.is_error()) {
                                            auto message = result.error().message();
                                            flash_error("Could not open plugin editor: %.*s", message.size(), message.data());
                                        } else {
                                            auto plugin_window = result.release_value();
                                            application.add_child_window(plugin_window);
                                            if (!plugin.open_editor(plugin_window.native_handle())) {
                                                auto name = plugin.name().or_else("<noname>"sv);
                                                fprintf(stderr, "ERROR: Could not open editor for plugin: '%.*s'\n", name.size(), name.data());
                                            }
                                        }
                                    }
                                }
                            }
                            break;

                            case FT_OTHER: {
                                flash_error("%s is neither a regular file nor a directory. We can't open it.", file_path);
                            }
                            break;

                            default:
                                UNREACHABLE("unknown File_Type");
                            }
                        }
                    }

                } else if (button.pressed) {
                    ui.fill_rect(box, vec4f(0.2f, 0.5f, 0.5f, 0.25));
                } else if (button.hovered) {
                    ui.fill_rect(box, vec4f(1.0f, 1.0f, 1.0f, 0.25));
                    fb.cursor = row;
                }
            }
        }

        // Info bar
        {
            ui.outline_rect({
                .box = vec4fv(
                    vec2fs(0.0f),
                    vec2f(space.x, 32.0f)
                ),
                .outline_size = border_size,
                .fill_color = info_bar_color,
                .top_color = info_bar_border_color,
            });
        }

        ui.end_frame();
    };

    auto plugin_window = Optional<UI::Window>();
    if (plugin && plugin->has_editor()) {
        auto rect = plugin->editor_rectangle().or_else(Vst::Rectangle{0, 0, 800, 600});
        plugin_window = TRY(UI::Window::create(plugin->name().or_else(""sv), rect.x, rect.y, rect.width, rect.height));
        if (!plugin->open_editor(plugin_window->native_handle())) {
            auto name = plugin->name().or_else("<noname>"sv);
            fprintf(stderr, "ERROR: Could not open editor for plugin: '%.*s'\n", name.size(), name.data());
        }
    }

    application.run();
    return 0;
}

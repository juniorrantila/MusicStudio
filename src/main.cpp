#include "./Common.h"
#include "./FileBrowser.h"
#include <Vst/Rectangle.h>

#include <Main/Main.h>

#include <Rexim/StringView.h>
#include <Rexim/LA.h>
#include <Rexim/Util.h>

#include <UI/Window.h>
#include <UI/UI.h>
#include <UI/FreeGlyph.h>
#include <UI/SimpleRenderer.h>

#include <MS/Plugin.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <GL/glew.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

void MessageCallback(GLenum source,
                     GLenum type,
                     GLuint id,
                     GLenum severity,
                     GLsizei length,
                     const GLchar* message,
                     const void* userParam)
{
    (void) source;
    (void) id;
    (void) length;
    (void) userParam;
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
}

static FileBrowser fb = {};

// TODO: display errors reported via flash_error right in the editor window somehow
#define flash_error(fmt, ...) do { fprintf(stderr, "ERROR: " fmt "\n", ## __VA_ARGS__); } while(0)

typedef struct {
    bool quit;
    bool is_fullscreen;
    SDL_Window* window;
} Handle_Events;
static void handle_events(Handle_Events*, UI::SimpleRenderer*);

ErrorOr<int> Main::main(int argc, c_string argv[])
{
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
    const char *const font_file_path = "./Fonts/OxaniumLight/Oxanium-Light.ttf";

    FT_Face face;
    error = FT_New_Face(library, font_file_path, 0, &face);
    if (error == FT_Err_Unknown_File_Format) {
        fprintf(stderr, "ERROR: `%s` has an unknown format\n", font_file_path);
        return 1;
    } else if (error) {
        fprintf(stderr, "ERROR: Could not load file `%s`\n", font_file_path);
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

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "ERROR: Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window =
        SDL_CreateWindow("MusicStudio",
                         0, 0,
                         SCREEN_WIDTH, SCREEN_HEIGHT,
                         SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (window == NULL) {
        fprintf(stderr, "ERROR: Could not create SDL window: %s\n", SDL_GetError());
        return 1;
    }

    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        int major;
        int minor;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
        printf("GL version %d.%d\n", major, minor);
    }

    if (SDL_GL_CreateContext(window) == NULL) {
        fprintf(stderr, "ERROR: Could not create OpenGL context: %s\n", SDL_GetError());
        return 1;
    }

    GLenum gl_error = 0;
    if (GLEW_OK != (gl_error = glewInit()) && gl_error != GLEW_ERROR_NO_GLX_DISPLAY) {
        fprintf(stderr, "ERROR: Could not initialize GLEW! %d\n", gl_error);
        return 1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (GLEW_ARB_debug_output) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(MessageCallback, 0);
    } else {
        fprintf(stderr, "WARNING: GLEW_ARB_debug_output is not available\n");
    }

    auto sr = TRY(UI::SimpleRenderer::create());
    {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        sr.set_resolution(vec2f(w, h));
        sr.set_camera_pos(vec2f(w, h) / 2.0f);
    }
    auto atlas = TRY(UI::FreeGlyphAtlas::create(face));

    auto plugin_window = Optional<UI::Window>();
    if (plugin && plugin->has_editor()) {
        auto rect = plugin->editor_rectangle().or_else(Vst::Rectangle{0, 0, 800, 600});
        plugin_window = TRY(UI::Window::create(plugin->name().or_else(""sv), rect.x, rect.y, rect.width, rect.height));
        if (!plugin->open_editor(plugin_window->native_handle())) {
            auto name = plugin->name().or_else("<noname>"sv);
            fprintf(stderr, "ERROR: Could not open editor for plugin: '%.*s'\n", name.size(), name.data());
        }
    }

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

    f32 border_size = 2.0f;

    auto ui = UI::UI(&sr, &atlas);
    Handle_Events context = (Handle_Events){
        .quit = false,
        .is_fullscreen = false,
        .window = window,
    };
    while (!context.quit) {
        handle_events(&context, &sr);
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

            if (fb.cursor < fb.files.count) {
                const Vec2f begin = vec2f(indent_size + 2.0f, space.y -((f32)fb.cursor + CURSOR_OFFSET + 1) * FREE_GLYPH_FONT_SIZE);
                Vec2f end = begin;
                StringView file_name = StringView::from_c_string(fb.files.items[fb.cursor].name);
                end += ui.measure_text(file_name);
                if (fb.files.items[fb.cursor].type == FT_DIRECTORY) {
                    end += ui.measure_text("/"sv);
                }
                auto box = vec4fv(begin, vec2f(end.x - begin.x, FREE_GLYPH_FONT_SIZE));
                ui.fill_rect(box, vec4f(1.0f, 1.0f, 1.0f, 0.25));
            }

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
        SDL_GL_SwapWindow(window);
    }

    return 0;
}

static void handle_events_browse_mode(SDL_Event);
static void handle_events(Handle_Events *context, UI::SimpleRenderer *sr)
{
    SDL_Event event = {0};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT)
            context->quit = true;

        switch (event.type) {

        case SDL_WINDOWEVENT: {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_RESTORED:
            case SDL_WINDOWEVENT_MAXIMIZED:
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED: {
                int w, h;
                SDL_GetWindowSize(context->window, &w, &h);
                glViewport(0, 0, w, h);
            }
            break;
            }
        }
        break;

        case SDL_KEYDOWN: {
            switch (event.key.keysym.sym) {
                case SDLK_F11: {
                    context->is_fullscreen = !context->is_fullscreen;
                    SDL_SetWindowFullscreen(context->window, context->is_fullscreen * SDL_WINDOW_FULLSCREEN_DESKTOP);
                }
                continue;

                case SDLK_F5: {
                    sr->reload_shaders().or_else([](Error error) {
                        auto message = error.message();
                        flash_error("Could not reload shaders: %.*s", message.size(), message.data());
                    });
                }
                continue;
            }
        } break;

        }

        handle_events_browse_mode(event);
    }
}
static void handle_events_browse_mode(SDL_Event event)
{
    switch (event.type) {
    case SDL_KEYDOWN: {
        switch (event.key.keysym.sym) {
        case SDLK_k:
        case SDLK_UP: {
            if (fb.cursor > 0) fb.cursor -= 1;
        }
        break;

        case SDLK_j:
        case SDLK_DOWN: {
            if (fb.cursor + 1 < fb.files.count) fb.cursor += 1;
        }
        break;

        case SDLK_RETURN: {
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
        }
        break;
        }
    } break;
    }
}

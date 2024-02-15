#include "./Common.h"
#include "./FileBrowser.h"
#include "./FreeGlyph.h"
#include "./SimpleRenderer.h"

#include <Main/Main.h>

#include <Rexim/StringView.h>
#include <Rexim/LA.h>
#include <Rexim/Util.h>

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

static FreeGlyphAtlas atlas = {};
static SimpleRenderer sr = {};
static FileBrowser fb = {};

// TODO: display errors reported via flash_error right in the editor window somehow
#define flash_error(fmt, ...) do { fprintf(stderr, fmt "\n", __VA_ARGS__); fprintf(stderr, "\n"); } while(0)

typedef struct {
    bool quit;
    bool is_fullscreen;
    SDL_Window* window;
} Handle_Events;
static void handle_events(Handle_Events*, SimpleRenderer*);

namespace Main {

ErrorOr<int> main(int argc, c_string argv[])
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
                assert(false && "loading files is not implemented");

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

    simple_renderer_init(&sr);
    free_glyph_atlas_init(&atlas, face);

    Handle_Events context = (Handle_Events){
        .quit = false,
        .is_fullscreen = false,
        .window = window,
    };
    while (!context.quit) {
        const Uint32 start = SDL_GetTicks();
        handle_events(&context, &sr);

        Vec4f bg = hex_to_vec4f(0x181818FF);
        glClearColor(bg.x, bg.y, bg.z, bg.w);
        glClear(GL_COLOR_BUFFER_BIT);

        fb_render(&fb, window, &atlas, &sr);

        SDL_GL_SwapWindow(window);

        const Uint32 duration = SDL_GetTicks() - start;
        const Uint32 delta_time_ms = 1000 / FPS;
        if (duration < delta_time_ms) {
            SDL_Delay(delta_time_ms - duration);
        }
    }

    return 0;
}

}

static void handle_events_browse_mode(SDL_Event);
static void handle_events(Handle_Events *context, SimpleRenderer *sr)
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
                int w = event.window.data1;
                int h = event.window.data2;
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
                    simple_renderer_reload_shaders(sr);
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
                        assert(false && "loading files is not implemented");
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

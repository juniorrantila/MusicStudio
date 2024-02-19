#include "./FileBrowser.h"
#include "./Common.h"

#include <UI/FreeGlyph.h>
#include <UI/SimpleRenderer.h>

#include <Rexim/Util.h>
#include <Rexim/StringView.h>

#include <SDL2/SDL.h>

static int file_cmp(const void *ap, const void *bp)
{
    c_string a = *(c_string*)ap;
    c_string b = *(c_string*)bp;
    return strcmp(a, b);
}

Errno fb_open_dir(FileBrowser* fb, c_string dir_path)
{
    fb->files.count = 0;
    fb->cursor = 0;
    Errno err = read_entire_dir(dir_path, &fb->files);
    if (err != 0) {
        return err;
    }
    qsort(fb->files.items, fb->files.count, sizeof(*fb->files.items), file_cmp);

    fb->dir_path.count = 0;
    sb_append_cstr(&fb->dir_path, dir_path);
    sb_append_null(&fb->dir_path);

    return 0;
}

#define PATH_SEP "/"
#define PATH_EMPTY ""
#define PATH_DOT "."
#define PATH_DOTDOT ".."

typedef struct {
    String_View* items;
    usize count;
    usize capacity;
} Comps;

void normpath(String_View path, String_Builder *result)
{
    usize original_sb_size = result->count;

    if (path.count == 0) {
        sb_append_cstr(result, PATH_DOT);
        return;
    }

    int initial_slashes = 0;
    while (path.count > 0 && *path.data == *PATH_SEP) {
        initial_slashes += 1;
        sv_chop_left(&path, 1);
    }
    if (initial_slashes > 2) {
        initial_slashes = 1;
    }

    Comps new_comps = {};

    while (path.count > 0) {
        String_View comp = sv_chop_by_delim(&path, '/');
        if (comp.count == 0 || sv_eq(comp, SV(PATH_DOT))) {
            continue;
        }
        if (!sv_eq(comp, SV(PATH_DOTDOT))) {
            da_append(&new_comps, comp);
            continue;
        }
        if (initial_slashes == 0 && new_comps.count == 0) {
            da_append(&new_comps, comp);
            continue;
        }
        if (new_comps.count > 0 && sv_eq(da_last(&new_comps), SV(PATH_DOTDOT))) {
            da_append(&new_comps, comp);
            continue;
        }
        if (new_comps.count > 0) {
            new_comps.count -= 1;
            continue;
        }
    }

    for (int i = 0; i < initial_slashes; ++i) {
        sb_append_cstr(result, PATH_SEP);
    }

    for (usize i = 0; i < new_comps.count; ++i) {
        if (i > 0) sb_append_cstr(result, PATH_SEP);
        sb_append_buf(result, new_comps.items[i].data, new_comps.items[i].count);
    }

    if (original_sb_size == result->count) {
        sb_append_cstr(result, PATH_DOT);
    }

    free(new_comps.items);
}

Errno fb_change_dir(FileBrowser *fb)
{
    assert(fb->dir_path.count > 0 && "You need to call fb_open_dir() before fb_change_dir()");
    assert(fb->dir_path.items[fb->dir_path.count - 1] == '\0');

    if (fb->cursor >= fb->files.count) return 0;

    c_string dir_name = fb->files.items[fb->cursor].name;

    fb->dir_path.count -= 1;

    // TODO: fb->dir_path grows indefinitely if we hit the root
    sb_append_cstr(&fb->dir_path, "/");
    sb_append_cstr(&fb->dir_path, dir_name);

    String_Builder result = {};
    normpath(sb_to_sv(fb->dir_path), &result);
    da_move(&fb->dir_path, result);
    sb_append_null(&fb->dir_path);

    printf("Changed dir to %s\n", fb->dir_path.items);

    fb->files.count = 0;
    fb->cursor = 0;
    Errno err = read_entire_dir(fb->dir_path.items, &fb->files);

    if (err != 0) {
        return err;
    }
    qsort(fb->files.items, fb->files.count, sizeof(*fb->files.items), file_cmp);

    return 0;
}

void fb_render(const FileBrowser *fb, SDL_Window *window, UI::FreeGlyphAtlas *atlas, UI::SimpleRenderer *sr)
{
    Vec2f cursor_pos = vec2f(0, -(f32)fb->cursor * FREE_GLYPH_FONT_SIZE);

    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    f32 max_line_len = 0.0f;

    sr->set_resolution(vec2f(w, h));
    sr->set_time((f32) SDL_GetTicks() / 1000.0f);

    sr->set_shader(UI::SHADER_FOR_COLOR);
    if (fb->cursor < fb->files.count) {
        const Vec2f begin = vec2f(0, -((f32)fb->cursor + CURSOR_OFFSET) * FREE_GLYPH_FONT_SIZE);
        Vec2f end = begin;
        auto file_name = StringView::from_c_string(fb->files.items[fb->cursor].name);
        end += atlas->measure_line_sized(file_name);
        if (fb->files.items[fb->cursor].type == FT_DIRECTORY) {
            atlas->render_line_sized(sr, "/"sv, &end, vec4fs(0.0f));
        }
        sr->solid_rect(begin, vec2f(end.x - begin.x, FREE_GLYPH_FONT_SIZE), vec4f(.25, .25, .25, 1));
    }
    sr->flush();

    sr->set_shader(UI::SHADER_FOR_EPICNESS);
    for (usize row = 0; row < fb->files.count; ++row) {
        const Vec2f begin = vec2f(0, -(f32)row * FREE_GLYPH_FONT_SIZE);
        Vec2f end = begin;
        auto file_name = StringView::from_c_string(fb->files.items[row].name);
        atlas->render_line_sized(sr, file_name, &end, vec4fs(0.0f));
        if (fb->files.items[row].type == FT_DIRECTORY) {
            atlas->render_line_sized(sr, "/"sv, &end, vec4fs(0.0f));
        }
        // TODO: the max_line_len should be calculated based on what's visible on the screen right now
        f32 line_len = fabsf(end.x - begin.x);
        if (line_len > max_line_len) {
            max_line_len = line_len;
        }
    }

    sr->flush();

    // Update camera
    {
        if (max_line_len > 1000.0f) {
            max_line_len = 1000.0f;
        }

        f32 target_scale = w/3/(max_line_len*0.75); // TODO: division by 0

        Vec2f target = cursor_pos;
        f32 offset = 0.0f;

        if (target_scale > 3.0f) {
            target_scale = 3.0f;
        } else {
            offset = cursor_pos.x - w/3/sr->camera_scale();
            if (offset < 0.0f) offset = 0.0f;
            target = vec2f(w/3/sr->camera_scale() + offset, cursor_pos.y);
        }

        sr->set_camera_vel((target - sr->camera_pos()) * 2.0f);
        sr->set_camera_scale_vel((target_scale - sr->camera_scale()) * 2.0f);

        sr->set_camera_pos(sr->camera_pos() + sr->camera_vel() * vec2fs(DELTA_TIME));
        sr->set_camera_scale(sr->camera_scale() + sr->camera_scale_vel() * DELTA_TIME);
    }
}

c_string fb_file_path(FileBrowser *fb)
{
    assert(fb->dir_path.count > 0 && "You need to call fb_open_dir() before fb_file_path()");
    assert(fb->dir_path.items[fb->dir_path.count - 1] == '\0');

    if (fb->cursor >= fb->files.count) return NULL;

    fb->file_path.count = 0;
    sb_append_buf(&fb->file_path, fb->dir_path.items, fb->dir_path.count - 1);
    sb_append_buf(&fb->file_path, "/", 1);
    sb_append_cstr(&fb->file_path, fb->files.items[fb->cursor].name);
    sb_append_null(&fb->file_path);

    return fb->file_path.items;
}

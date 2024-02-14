#pragma once
#include "./Forward.h"

#include <Ty/Base.h>
#include <Rexim/LA.h>
#include <GL/glew.h>

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define FREE_GLYPH_FONT_SIZE 128

// https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Text_Rendering_02

struct GlyphMetric {
    f32 ax; // advance.x
    f32 ay; // advance.y

    f32 bw; // bitmap.width;
    f32 bh; // bitmap.rows;

    f32 bl; // bitmap_left;
    f32 bt; // bitmap_top;

    f32 tx; // x offset of glyph in texture coordinates
};

#define GLYPH_METRICS_CAPACITY 128

struct FreeGlyphAtlas {
    FT_UInt atlas_width;
    FT_UInt atlas_height;
    GLuint glyphs_texture;
    GlyphMetric metrics[GLYPH_METRICS_CAPACITY];
};

void free_glyph_atlas_init(FreeGlyphAtlas* atlas, FT_Face face);
f32 free_glyph_atlas_cursor_pos(FreeGlyphAtlas const* atlas, c_string text, usize text_size, Vec2f pos, usize col);
void free_glyph_atlas_measure_line_sized(FreeGlyphAtlas *atlas, c_string text, usize text_size, Vec2f *pos);
void free_glyph_atlas_render_line_sized(FreeGlyphAtlas *atlas, SimpleRenderer *sr, c_string text, usize text_size, Vec2f *pos, Vec4f color);

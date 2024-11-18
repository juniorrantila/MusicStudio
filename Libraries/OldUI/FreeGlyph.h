#pragma once
#include "./Forward.h"

#include <Ty/Base.h>
#include <Ty/ErrorOr.h>

#include <Rexim/LA.h>

#include "./Graphics/GL.h"

#include <ft2build.h>
#include FT_FREETYPE_H

// https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Text_Rendering_02

namespace UI {

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
    static ErrorOr<FreeGlyphAtlas> create(FT_Face face);
    void load();

    f32 cursor_pos(StringView text, Vec2f pos, usize col) const;
    Vec2f measure_line_sized(StringView text) const;
    void render_line_sized(SimpleRenderer* sr, StringView text, Vec4f box, Vec4f color);

    FT_UInt m_atlas_width { 0 };
    FT_UInt m_atlas_height { 0 };
    GLuint m_glyphs_texture { 0 };
    GlyphMetric* m_metrics { nullptr };
    usize m_metrics_capacity { GLYPH_METRICS_CAPACITY };
};

}

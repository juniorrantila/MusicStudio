#include "./FreeGlyph.h"
#include "./SimpleRenderer.h"

#include <assert.h>

namespace UI {

void FreeGlyphAtlas::load()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_glyphs_texture);
}

ErrorOr<FreeGlyphAtlas> FreeGlyphAtlas::create(FT_Face face)
{
    auto atlas = FreeGlyphAtlas();
    atlas.m_metrics = (GlyphMetric*)calloc(atlas.m_metrics_capacity, sizeof(GlyphMetric));
    if (!atlas.m_metrics) {
        return Error::from_errno();
    }

    // TODO: Introduction of SDF font slowed down the start up time
    // We need to investigate what's up with that
    FT_Int32 load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_(FT_RENDER_MODE_SDF);
    for (int i = 32; i < 128; ++i) {
        if (FT_Load_Char(face, i, load_flags)) {
            fprintf(stderr, "ERROR: could not load glyph of a character with code %d\n", i);
            exit(1);
        }

        atlas.m_atlas_width += face->glyph->bitmap.width;
        if (atlas.m_atlas_height < face->glyph->bitmap.rows) {
            atlas.m_atlas_height = face->glyph->bitmap.rows;
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &atlas.m_glyphs_texture);
    glBindTexture(GL_TEXTURE_2D, atlas.m_glyphs_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        (GLsizei) atlas.m_atlas_width,
        (GLsizei) atlas.m_atlas_height,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        NULL);

    int x = 0;
    for (int i = 32; i < 128; ++i) {
        if (FT_Load_Char(face, i, load_flags)) {
            fprintf(stderr, "ERROR: could not load glyph of a character with code %d\n", i);
            return Error::from_string_literal("could not load glyph");
        }

        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
            fprintf(stderr, "ERROR: could not render glyph of a character with code %d\n", i);
            return Error::from_string_literal("could not render glyph");
        }

        atlas.m_metrics[i].ax = face->glyph->advance.x >> 6;
        atlas.m_metrics[i].ay = face->glyph->advance.y >> 6;
        atlas.m_metrics[i].bw = face->glyph->bitmap.width;
        atlas.m_metrics[i].bh = face->glyph->bitmap.rows;
        atlas.m_metrics[i].bl = face->glyph->bitmap_left;
        atlas.m_metrics[i].bt = face->glyph->bitmap_top;
        atlas.m_metrics[i].tx = (f32) x / (f32) atlas.m_atlas_width;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            x,
            0,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer);
        x += face->glyph->bitmap.width;
    }

    return atlas;
}

f32 FreeGlyphAtlas::cursor_pos(StringView text, Vec2f pos, usize col) const
{
    for (usize i = 0; i < text.size(); ++i) {
        if (i == col) {
            return pos.x;
        }

        usize glyph_index = text[i];
        if (glyph_index >= GLYPH_METRICS_CAPACITY) {
            glyph_index = '?';
        }

        GlyphMetric metric = m_metrics[glyph_index];
        pos.x += metric.ax;
        pos.y += metric.ay;
    }

    return pos.x;
}

Vec2f FreeGlyphAtlas::measure_line_sized(StringView text) const
{
    Vec2f pos = vec2fs(0.0f);
    for (usize i = 0; i < text.size(); ++i) {
        usize glyph_index = text[i];
        // TODO: support for glyphs outside of ASCII range
        if (glyph_index >= GLYPH_METRICS_CAPACITY) {
            glyph_index = '?';
        }
        GlyphMetric metric = m_metrics[glyph_index];

        pos.x += metric.ax;
        pos.y += metric.ay;
    }
    return pos;
}

void FreeGlyphAtlas::render_line_sized(SimpleRenderer *sr, StringView text, Vec2f *pos, Vec4f color)
{
    for (usize i = 0; i < text.size(); ++i) {
        usize glyph_index = text[i];
        // TODO: support for glyphs outside of ASCII range
        if (glyph_index >= GLYPH_METRICS_CAPACITY) {
            glyph_index = '?';
        }
        GlyphMetric metric = m_metrics[glyph_index];
        f32 x2 = pos->x + metric.bl;
        f32 y2 = -pos->y - metric.bt;
        f32 w  = metric.bw;
        f32 h  = metric.bh;

        pos->x += metric.ax;
        pos->y += metric.ay;

        sr->image_rect(
            vec2f(x2, -y2),
            vec2f(w, -h),
            vec2f(metric.tx, 0.0f),
            vec2f(metric.bw / (f32) m_atlas_width, metric.bh / (f32) m_atlas_height),
            color);
    }
}

}

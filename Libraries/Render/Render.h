#pragma once
#include "./Forward.h"

#include <Ty/Base.h>
#include <Ty/Allocator.h>
#include <FS/FSVolume.h>
#include <Ty2/Logger.h>
#include <Rexim/LA.h>

typedef struct {
    FileID vert;
    FileID frag;
} RenderShader;

C_API c_string render_strerror(int);
C_API Render* render_create(FSVolume const*, Allocator* gpa, Logger*);
C_API void render_destroy(Render*);

C_API void render_set_time(Render*, f32 time);
C_API void render_set_resolution(Render*, Vec2f);
C_API void render_set_mouse_position(Render*, Vec2f);
C_API bool render_use_shader(Render*, RenderShader);
C_API void render_update(Render*);
C_API bool render_needs_reload(Render const*);
C_API bool render_reload(Render*);
C_API void render_flush(Render*);

C_API void render_clear(Render*, Vec4f color);

C_API void render_triangle(Render*,
    Vec2f p0, Vec4f c0, Vec2f uv0,
    Vec2f p1, Vec4f c1, Vec2f uv1,
    Vec2f p2, Vec4f c2, Vec2f uv2
);

// 0-1
// |/|
// 2-3
C_API void render_quad(Render*,
    Vec2f p0, Vec4f c0, Vec2f uv0,
    Vec2f p1, Vec4f c1, Vec2f uv1,
    Vec2f p2, Vec4f c2, Vec2f uv2,
    Vec2f p3, Vec4f c3, Vec2f uv3
);

C_API void render_cursor(Render* render, Vec4f color);

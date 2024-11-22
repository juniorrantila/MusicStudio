#pragma once
#include "./Forward.h"

#include <Ty/Base.h>
#include <Ty/Allocator.h>
#include <FS/Bundle.h>
#include <Rexim/LA.h>

c_string render_strerror(int);
Render* render_create(FS::Bundle const* bundle, Ty::Allocator* gpa);
void render_destroy(Render*);

void render_set_time(Render*, f32 time);
void render_set_resolution(Render*, Vec2f);
void render_set_mouse_position(Render*, Vec2f);
int render_reload_shaders(Render*);
void render_flush(Render*);

void render_clear(Render*, Vec4f color);

void render_transact(Render* render, usize vertices);
void render_vertex(Render*, Vec2f position, Vec4f color, Vec2f uv);

void render_triangle(Render*,
    Vec2f p0, Vec4f c0, Vec2f uv0,
    Vec2f p1, Vec4f c1, Vec2f uv1,
    Vec2f p2, Vec4f c2, Vec2f uv2
);

void render_cursor(Render* render, Vec4f color);

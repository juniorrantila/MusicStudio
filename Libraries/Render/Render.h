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
void render_set_camera_scale(Render*, f32 scale);
void render_set_camera_position(Render*, Vec2f);
void render_set_camera_velocity(Render*, Vec2f);
int render_reload_shaders(Render*);
void render_flush(Render*);

void render_clear(Render*, Vec4f color);

void render_solid_rect(Render*, Vec2f point, Vec2f size, Vec4f color);
void render_outline_rect(Render*, Vec2f point, Vec2f size, f32 outline_size, Vec4f fill_color, Vec4f outline_color);

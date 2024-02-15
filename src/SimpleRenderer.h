#pragma once
#include <GL/glew.h>

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#include <Rexim/LA.h>

enum UniformSlot {
    UNIFORM_SLOT_TIME = 0,
    UNIFORM_SLOT_RESOLUTION,
    UNIFORM_SLOT_CAMERA_POS,
    UNIFORM_SLOT_CAMERA_SCALE,
    COUNT_UNIFORM_SLOTS,
};

enum SimpleVertexAttr {
    SIMPLE_VERTEX_ATTR_POSITION = 0,
    SIMPLE_VERTEX_ATTR_COLOR,
    SIMPLE_VERTEX_ATTR_UV,
};

struct SimpleVertex {
    Vec2f position;
    Vec4f color;
    Vec2f uv;
};

#define SIMPLE_VERTICIES_CAP (3*640*1000)

static_assert(SIMPLE_VERTICIES_CAP%3 == 0, "Simple renderer vertex capacity must be divisible by 3. We are rendring triangles after all.");

enum SimpleShader {
    SHADER_FOR_COLOR = 0,
    SHADER_FOR_IMAGE,
    SHADER_FOR_TEXT,
    SHADER_FOR_EPICNESS, // This is the one that does that cool rainbowish animation
    COUNT_SIMPLE_SHADERS,
};

struct SimpleRenderer {
    GLuint vao;
    GLuint vbo;
    GLuint programs[COUNT_SIMPLE_SHADERS];
    SimpleShader current_shader;

    GLint uniforms[COUNT_UNIFORM_SLOTS];
    SimpleVertex verticies[SIMPLE_VERTICIES_CAP];
    usize verticies_count;

    Vec2f resolution;
    f32 time;

    Vec2f camera_pos;
    f32 camera_scale;
    f32 camera_scale_vel;
    Vec2f camera_vel;

    Vec2f cursor_pos;
    f32 cursor_absolute_pos_x;
    f32 cursor_scale_vel;
    Vec2f cursor_vel;
};

void simple_renderer_init(SimpleRenderer*);

void simple_renderer_reload_shaders(SimpleRenderer*);

void simple_renderer_vertex(SimpleRenderer*, Vec2f p, Vec4f c, Vec2f uv);
void simple_renderer_set_shader(SimpleRenderer *sr, SimpleShader shader);
void simple_renderer_triangle(SimpleRenderer*,
                              Vec2f p0, Vec2f p1, Vec2f p2,
                              Vec4f c0, Vec4f c1, Vec4f c2,
                              Vec2f uv0, Vec2f uv1, Vec2f uv2);
void simple_renderer_quad(SimpleRenderer*,
                          Vec2f p0, Vec2f p1, Vec2f p2, Vec2f p3,
                          Vec4f c0, Vec4f c1, Vec4f c2, Vec4f c3,
                          Vec2f uv0, Vec2f uv1, Vec2f uv2, Vec2f uv3);
void simple_renderer_outline_rect(SimpleRenderer*, Vec2f p, Vec2f size, f32 outline_size, Vec4f fill_color, Vec4f outline_color);

struct SimpleRendererOutlineRectEx {
    Vec2f point;
    Vec2f size;
    f32 outline_size;
    Vec4f fill_color = vec4fs(0);
    Vec4f left_color = vec4fs(0);
    Vec4f top_color = vec4fs(0);
    Vec4f right_color = vec4fs(0);
    Vec4f bottom_color = vec4fs(0);
};
void simple_renderer_outline_rect_ex_impl(SimpleRenderer*, SimpleRendererOutlineRectEx);
#define simple_renderer_outline_rect_ex(sr, ...) simple_renderer_outline_rect_ex_impl(sr, { __VA_ARGS__ })

void simple_renderer_solid_rect(SimpleRenderer*, Vec2f p, Vec2f s, Vec4f c);
void simple_renderer_image_rect(SimpleRenderer*, Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, Vec4f c);
void simple_renderer_flush(SimpleRenderer*);
void simple_renderer_sync(SimpleRenderer*);
void simple_renderer_draw(SimpleRenderer*);

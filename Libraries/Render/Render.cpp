#include "./Render.h"

#include <GL/GL.h>
#include <FS/Bundle.h>
#include <Ty/Allocator.h>
#include <Ty/View.h>
#include <string.h>

enum UniformSlot {
    UniformSlot_Time,
    UniformSlot_Resolution,
    UniformSlot_CameraPos,
    UniformSlot_CameraScale,
    UniformSlot__Count,
};

enum VertexAttr {
    VertexAttr_Position,
    VertexAttr_Color,
    VertexAttr_UV,
};

struct Vertex {
    Vec2f position;
    Vec4f color;
    Vec2f uv;
};

enum Shader {
    Shader_Color,
    Shader_Image,
    Shader_Text,
    Shader_Epic,
    Shader__COUNT,
};

struct Render {
    Ty::Allocator* gpa;
    FS::Bundle const* bundle;

    GLuint vao;
    GLuint vbo;
    GLuint programs[Shader__COUNT];
    GLuint uniforms[UniformSlot__Count];
    Shader current_shader;

    usize vertex_index;
    View<Vertex> vertices;

    f32 time;
    Vec2f resolution;

    Vec2f camera_position;
    f32 camera_scale;
    f32 camera_scale_velocity;
    Vec2f camera_velocity;
};

c_string render_strerror(int error)
{
    if (error == 0) return "no error";
    return "unknown error";
}

Render* render_create(FS::Bundle const* bundle, Ty::Allocator* gpa)
{
    Render* render = nullptr;
    {
        auto result = gpa->alloc<Render>();
        if (result.is_error()) {
            return nullptr;
        }
        render = result.release_value();
    }
    memset(render, 0, sizeof(*render));
    render->gpa = gpa;
    render->bundle = bundle;
    render->current_shader = (Shader)-1;
    {
        auto result = gpa->alloc<Vertex>(4096);
        if (result.is_error()) {
            gpa->free(render);
            return nullptr;
        }
        render->vertices = result.release_value();
    }

    return render;
}

void render_destroy(Render* render)
{
    render->gpa->free(render->vertices);
    render->gpa->free(render);
}

void render_set_time(Render* render, f32 time)
{
    render->time = time;
}

void render_set_resolution(Render* render, Vec2f resolution)
{
    render->resolution = resolution;
}

void render_set_camera_scale(Render* render, f32 scale)
{
    render->camera_scale = scale;
}

void render_set_camera_position(Render* render, Vec2f pos)
{
    render->camera_position = pos;
}

void render_set_camera_velocity(Render* render, Vec2f vel)
{
    render->camera_velocity = vel;
}

int render_reload_shaders(Render*)
{
    return 1;
}

void render_flush(Render*)
{
}

void render_clear(Render* render, Vec4f color)
{
    glViewport(0, 0, render->resolution.x, render->resolution.y);
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void render_solid_rect(Render*, Vec2f point, Vec2f size, Vec4f color)
{
    (void)point;
    (void)size;
    (void)color;
}

void render_outline_rect(Render*, Vec2f point, Vec2f size, f32 outline_size, Vec4f fill_color, Vec4f outline_color)
{
    (void)point;
    (void)size;
    (void)outline_size;
    (void)fill_color;
    (void)outline_color;
}

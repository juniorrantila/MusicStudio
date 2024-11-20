#include "./Render.h"

#include <GL/GL.h>
#include <FS/Bundle.h>
#include <Ty/Allocator.h>
#include <Ty/View.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

struct Vertex {
    Vec2f position;
    Vec4f color;
    Vec2f uv;
};

enum UniformSlot {
    UniformSlot_Time,
    UniformSlot_Resolution,
    UniformSlot__Count,
};

enum VertexAttr {
    VertexAttr_Position,
    VertexAttr_Color,
    VertexAttr_UV,
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
};

static int compile_shader_source(StringView, GLenum type);
static int link_program(GLuint program);
static void uniform_location(GLuint program, GLuint locations[UniformSlot__Count]);

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

    {
        glGenVertexArrays(1, &render->vao);
        glBindVertexArray(render->vao);

        glGenBuffers(1, &render->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, render->vbo);
        glBufferData(GL_ARRAY_BUFFER, render->vertices.size() * sizeof(Vertex), render->vertices.data(), GL_DYNAMIC_DRAW);

        // position
        glEnableVertexAttribArray(VertexAttr_Position);
        glVertexAttribPointer(
            VertexAttr_Position,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid *) offsetof(Vertex, position));

        // color
        glEnableVertexAttribArray(VertexAttr_Color);
        glVertexAttribPointer(
            VertexAttr_Color,
            4,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid *) offsetof(Vertex, color));

        // uv
        glEnableVertexAttribArray(VertexAttr_UV);
        glVertexAttribPointer(
            VertexAttr_UV,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid *) offsetof(Vertex, uv));
    }
    if (render_reload_shaders(render) < 0) return 0;

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
    glUniform1f(render->uniforms[UniformSlot_Time], time);
}

void render_set_resolution(Render* render, Vec2f resolution)
{
    if (render->resolution != resolution) {
        render->resolution = resolution;
        glViewport(0, 0, render->resolution.x, render->resolution.y);
        glUniform2f(render->uniforms[UniformSlot_Resolution], resolution.x, resolution.y);
    }
}

int render_reload_shaders(Render* render)
{
    auto vert = render->bundle->open("Shaders/simple.vert");
    if (!vert) {
        fprintf(stderr, "could not open 'Shaders/simple.vert'\n");
        return -1;
    }
    auto vs = compile_shader_source(vert->view(), GL_VERTEX_SHADER);
    if (vs < 0) return -1;

    auto frag = render->bundle->open("Shaders/color.frag");
    if (!frag) {
        fprintf(stderr, "could not open 'Shaders/color.frag'\n");
        return -1;
    }
    auto fs = compile_shader_source(frag->view(), GL_FRAGMENT_SHADER);
    if (fs < 0) return -1;

    auto program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    if (link_program(program) < 0) return -1;

    render->programs[Shader_Color] = program;

    return 0;
}

void render_use_simple(Render* render)
{
    if (render->current_shader == Shader_Color)
        return;
    render_flush(render);
    render->current_shader = Shader_Color;
    glUseProgram(render->programs[Shader_Color]);
    uniform_location(render->programs[Shader_Color], render->uniforms);
    glUniform2f(render->uniforms[UniformSlot_Resolution], render->resolution.x, render->resolution.y);
    glUniform1f(render->uniforms[UniformSlot_Time], render->time);
}

void render_flush(Render* render)
{
    if (render->vertex_index != 0) {
        glBufferSubData(GL_ARRAY_BUFFER,
                        0,
                        render->vertex_index * sizeof(Vertex),
                        render->vertices.data());
        glDrawArrays(GL_TRIANGLES, 0, render->vertex_index);
        render->vertex_index = 0;
    }
}

void render_clear(Render* render, Vec4f color)
{
    glViewport(0, 0, render->resolution.x, render->resolution.y);
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void render_transact(Render* render, usize vertices)
{
    if (render->vertices.size() <= render->vertex_index + vertices) {
        render_flush(render);
    }
}

void render_vertex(Render* render, Vec2f position, Vec4f color, Vec2f uv)
{
    render_transact(render, 1);
    render->vertices[render->vertex_index++] = {
        .position = position,
        .color = color,
        .uv = uv,
    };
}

static const char *shader_type_as_cstr(GLuint shader)
{
    switch (shader) {
    case GL_VERTEX_SHADER:
        return "GL_VERTEX_SHADER";
    case GL_FRAGMENT_SHADER:
        return "GL_FRAGMENT_SHADER";
    default:
        return "(Unknown)";
    }
}

static int compile_shader_source(StringView source, GLenum shader_type)
{
    auto shader = glCreateShader(shader_type);
    auto data = (GLchar*)source.data();
    auto size = (GLint)source.size();
    glShaderSource(shader, 1, &data, &size);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLchar message[1024];
        GLsizei message_size = 0;
        glGetShaderInfoLog(shader, sizeof(message), &message_size, message);
        fprintf(stderr, "ERROR: could not compile %s\n", shader_type_as_cstr(shader_type));
        fprintf(stderr, "%.*s\n", message_size, message);
        return -1;
    }

    return shader;
}

static int link_program(GLuint program)
{
    glLinkProgram(program);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLsizei message_size = 0;
        GLchar message[1024];

        glGetProgramInfoLog(program, sizeof(message), &message_size, message);
        fprintf(stderr, "Program Linking: %.*s\n", message_size, message);
        return -1;
    }

    return 0;
}

struct UniformDef {
    UniformSlot slot;
    const char *name;
};

static_assert(UniformSlot__Count == 2, "The amount of the shader uniforms have change. Please update the definition table accordingly");
static const UniformDef uniform_defs[UniformSlot__Count] = {
    [UniformSlot_Time] = {
        .slot = UniformSlot_Time,
        .name = "time",
    },
    [UniformSlot_Resolution] = {
        .slot = UniformSlot_Resolution,
        .name = "resolution",
    },
};


static void uniform_location(GLuint program, GLuint locations[UniformSlot__Count])
{
    for (int slot = 0; slot < UniformSlot__Count; ++slot) {
        locations[slot] = glGetUniformLocation(program, uniform_defs[slot].name);
    }
}

#include "./Render.h"

#include <FS/FSVolume.h>
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
    u32 flags;
};

enum UniformSlot {
    UniformSlot_Time,
    UniformSlot_Resolution,
    UniformSlot_MousePosition,
    UniformSlot__Count,
};

enum VertexAttr {
    VertexAttr_Position,
    VertexAttr_Color,
    VertexAttr_UV,
    VertexAttr_Flags,
};

enum Shader {
    Shader_Color  = 0,
    Shader_Image  = 1,
    Shader_Text   = 2,
    Shader_Cursor = 3,
    Shader__COUNT,
};

struct Render {
    Allocator* gpa;
    FSVolume const* volume;

    GLuint texture;
    GLuint vao;
    GLuint vbo;
    GLuint shader_program;
    GLuint uniforms[UniformSlot__Count];

    usize vertex_index;
    View<Vertex> vertices;

    f32 time;
    Vec2f resolution;
    Vec2f mouse_position;
};

static int compile_shader_source(StringView, GLenum type);
static int link_program(GLuint program);
static void uniform_location(GLuint program, GLuint locations[UniformSlot__Count]);

static void transact(Render* render, usize vertices);
static void vertex(Render* render, Vec2f position, Vec4f color, Vec2f uv, u32 flags);
static void triangle(Render* render,
    Vec2f p0, Vec4f c0, Vec2f uv0, u32 f0,
    Vec2f p1, Vec4f c1, Vec2f uv1, u32 f1,
    Vec2f p2, Vec4f c2, Vec2f uv2, u32 f2
);

// 0-1
// |/|
// 2-3
static void quad(Render* render,
    Vec2f p0, Vec4f c0, Vec2f uv0, u32 f0,
    Vec2f p1, Vec4f c1, Vec2f uv1, u32 f1,
    Vec2f p2, Vec4f c2, Vec2f uv2, u32 f2,
    Vec2f p3, Vec4f c3, Vec2f uv3, u32 f3
);

C_API c_string render_strerror(int error)
{
    if (error == 0) return "no error";
    return "unknown error";
}

C_API Render* render_create(FSVolume const* volume, Allocator* gpa)
{
    Render* render = gpa->alloc<Render>(1);
    if (!render) return nullptr;
    memset(render, 0, sizeof(*render));
    render->gpa = gpa;
    render->volume = volume;

    usize vertex_count = 4096;
    auto* vertices = gpa->alloc<Vertex>(vertex_count);
    if (!vertices) {
        gpa->free(render, 1);
        return nullptr;
    }
    render->vertices = View(vertices, vertex_count);

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

        // flags
        glEnableVertexAttribArray(VertexAttr_Flags);
        glVertexAttribPointer(
            VertexAttr_Flags,
            1,
            GL_UNSIGNED_INT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid *) offsetof(Vertex, flags));

        glActiveTexture(GL_TEXTURE0);

        glGenTextures(1, &render->texture);
        glBindTexture(GL_TEXTURE_2D, render->texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            (GLsizei) 1,
            (GLsizei) 1,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            NULL);
    }
    if (render_reload_shaders(render) < 0) return 0;
    return render;
}

C_API void render_destroy(Render* render)
{
    render->gpa->free(render->vertices.data(), render->vertices.size());
    render->gpa->free(render, 1);
}

C_API void render_set_time(Render* render, f32 time)
{
    render->time = time;
    glUniform1f(render->uniforms[UniformSlot_Time], time);
}

C_API void render_set_resolution(Render* render, Vec2f resolution)
{
    if (render->resolution != resolution) {
        render->resolution = resolution;
        glViewport(0, 0, resolution.x, resolution.y);
        glUniform2f(render->uniforms[UniformSlot_Resolution], resolution.x, resolution.y);
    }
}

C_API void render_set_mouse_position(Render* render, Vec2f position)
{
    render->mouse_position = position;
    glUniform2f(render->uniforms[UniformSlot_MousePosition], position.x, position.y);
}

C_API int render_reload_shaders(Render* render)
{
    auto vert = render->volume->open("Shaders/simple.vert"s);
    if (!vert.has_value()) {
        (void)fprintf(stderr, "could not open 'Shaders/simple.vert'\n");
        return -1;
    }
    auto vs = compile_shader_source(vert->as_view(), GL_VERTEX_SHADER);
    if (vs < 0) return -1;

    auto frag = render->volume->open("Shaders/color.frag"s);
    if (!frag.has_value()) {
        (void)fprintf(stderr, "could not open 'Shaders/color.frag'\n");
        return -1;
    }
    auto fs = compile_shader_source(frag->as_view(), GL_FRAGMENT_SHADER);
    if (fs < 0) return -1;

    auto program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    if (link_program(program) < 0) return -1;

    render->shader_program = program;
    glUseProgram(render->shader_program);
    uniform_location(render->shader_program, render->uniforms);
    glUniform1f(render->uniforms[UniformSlot_Time], render->time);
    glUniform2f(render->uniforms[UniformSlot_Resolution], render->resolution.x, render->resolution.y);
    glUniform2f(render->uniforms[UniformSlot_MousePosition], render->mouse_position.x, render->mouse_position.y);

    return 0;
}

C_API int render_handle_events(Render* render, FSEvents events)
{
    for (usize i = 0; i < events.count; i++) {
        auto event = events.items[i];
        if (event.kind == FSEventKind_Modify) {
            return render_reload_shaders(render);
        }
    }
    return 0;
}

C_API void render_flush(Render* render)
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

C_API void render_clear(Render* render, Vec4f color)
{
    (void)render;
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void transact(Render* render, usize vertices)
{
    if (render->vertices.size() <= render->vertex_index + vertices) {
        render_flush(render);
    }
}

static void vertex(Render* render, Vec2f position, Vec4f color, Vec2f uv, u32 flags)
{
    transact(render, 1);
    render->vertices[render->vertex_index++] = {
        .position = position,
        .color = color,
        .uv = uv,
        .flags = flags,
    };
}

static void triangle(Render* render,
    Vec2f p0, Vec4f c0, Vec2f uv0, u32 f0,
    Vec2f p1, Vec4f c1, Vec2f uv1, u32 f1,
    Vec2f p2, Vec4f c2, Vec2f uv2, u32 f2)
{
    transact(render, 3);
    vertex(render, p0, c0, uv0, f0);
    vertex(render, p1, c1, uv1, f1);
    vertex(render, p2, c2, uv2, f2);
}

// 0-1
// |/|
// 2-3
static void quad(Render* render,
    Vec2f p0, Vec4f c0, Vec2f uv0, u32 f0,
    Vec2f p1, Vec4f c1, Vec2f uv1, u32 f1,
    Vec2f p2, Vec4f c2, Vec2f uv2, u32 f2,
    Vec2f p3, Vec4f c3, Vec2f uv3, u32 f3)
{
    triangle(render,
        p0, c0, uv0, f0,
        p1, c1, uv1, f1,
        p2, c2, uv2, f2
    );
    triangle(render,
        p1, c1, uv1, f1,
        p2, c2, uv2, f2,
        p3, c3, uv3, f3
    );
}

C_API void render_triangle(Render* render,
    Vec2f p0, Vec4f c0, Vec2f uv0,
    Vec2f p1, Vec4f c1, Vec2f uv1,
    Vec2f p2, Vec4f c2, Vec2f uv2)
{
    triangle(render,
        p0, c0, uv0, Shader_Color,
        p1, c1, uv1, Shader_Color,
        p2, c2, uv2, Shader_Color
    );
}

// 0-1
// |/|
// 2-3
C_API void render_quad(Render* render,
    Vec2f p0, Vec4f c0, Vec2f uv0,
    Vec2f p1, Vec4f c1, Vec2f uv1,
    Vec2f p2, Vec4f c2, Vec2f uv2,
    Vec2f p3, Vec4f c3, Vec2f uv3)
{
    render_triangle(render,
        p0, c0, uv0,
        p1, c1, uv1,
        p2, c2, uv2
    );
    render_triangle(render,
        p1, c1, uv1,
        p2, c2, uv2,
        p3, c3, uv3
    );
}

C_API void render_cursor(Render* render, Vec4f color)
{
    quad(render,
        vec2f(0.0f, 0.0f), color, vec2fs(0), Shader_Cursor,
        vec2f(1.0f, 0.0f), color, vec2fs(0), Shader_Cursor,
        vec2f(0.0f, 1.0f), color, vec2fs(0), Shader_Cursor,
        vec2f(1.0f, 1.0f), color, vec2fs(0), Shader_Cursor
    );
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

static_assert(UniformSlot__Count == 3, "The amount of the shader uniforms have change. Please update the definition table accordingly");
static const UniformDef uniform_defs[UniformSlot__Count] = {
    [UniformSlot_Time] = {
        .slot = UniformSlot_Time,
        .name = "time",
    },
    [UniformSlot_Resolution] = {
        .slot = UniformSlot_Resolution,
        .name = "resolution",
    },
    [UniformSlot_MousePosition] = {
        .slot = UniformSlot_MousePosition,
        .name = "mouse_position",
    },
};


static void uniform_location(GLuint program, GLuint locations[UniformSlot__Count])
{
    for (int slot = 0; slot < UniformSlot__Count; ++slot) {
        locations[slot] = glGetUniformLocation(program, uniform_defs[slot].name);
    }
}

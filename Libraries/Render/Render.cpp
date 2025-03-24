#include "./Render.h"

#include <Ty/Defer.h>
#include <FS/FSVolume.h>
#include <GL/GL.h>
#include <FS/Bundle.h>
#include <Ty/Allocator.h>
#include <Ty/View.h>

#include <string.h>

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
    Logger* log;

    GLuint texture;
    GLuint vao;
    GLuint vbo;

    RenderShader current_shaders;
    GLuint current_program;
    GLint uniforms[UniformSlot__Count];

    f32 time;
    Vec2f resolution;
    Vec2f mouse_position;

    bool needs_reload;

    usize vertex_index;
    usize vertex_capacity;
    Vertex vertices[];
};

static bool compile_shader_source(Logger* log, StringSlice, GLenum type, GLuint* out);
static bool link_program(Logger* log, GLuint program);
static void uniform_location(GLuint program, GLint locations[UniformSlot__Count]);

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

C_API Render* render_create(FSVolume const* volume, Allocator* gpa, Logger* log)
{
    usize vertex_capacity = 4096;
    Render* render = (Render*)memalloc(gpa, sizeof(Render) + vertex_capacity * sizeof(Vertex), alignof(Render));
    if (!render) return nullptr;
    Defer free_render = [&] {
        memfree(gpa, render, sizeof(Render) + vertex_capacity * sizeof(Vertex), alignof(Render));
    };
    memset(render, 0, sizeof(*render));
    render->gpa = gpa;
    render->volume = volume;
    render->log = log;
    render->vertex_capacity = vertex_capacity;
    {
        glGenVertexArrays(1, &render->vao);
        glBindVertexArray(render->vao);

        glGenBuffers(1, &render->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, render->vbo);
        glBufferData(GL_ARRAY_BUFFER, render->vertex_capacity * sizeof(Vertex), render->vertices, GL_DYNAMIC_DRAW);

        // position
        glEnableVertexAttribArray(VertexAttr_Position);
        glVertexAttribPointer(
            VertexAttr_Position,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid *) ty_offsetof(Vertex, position));

        // color
        glEnableVertexAttribArray(VertexAttr_Color);
        glVertexAttribPointer(
            VertexAttr_Color,
            4,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid *) ty_offsetof(Vertex, color));

        // uv
        glEnableVertexAttribArray(VertexAttr_UV);
        glVertexAttribPointer(
            VertexAttr_UV,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid *) ty_offsetof(Vertex, uv));

        // flags
        glEnableVertexAttribArray(VertexAttr_Flags);
        glVertexAttribPointer(
            VertexAttr_Flags,
            1,
            GL_UNSIGNED_INT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid *) ty_offsetof(Vertex, flags));

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

    auto vert = volume->find("Shaders/simple.vert"s);
    if (!vert.has_value()) {
        log->error("could not find 'Shaders/simple.vert'");
        return nullptr;
    }
    auto frag = volume->find("Shaders/color.frag"s);
    if (!frag.has_value()) {
        log->error("could not find 'Shaders/color.frag'");
        return nullptr;
    }
    auto shader = RenderShader {
        .vert = vert.value(),
        .frag = frag.value(),
    };

    if (!render_use_shader(render, shader)) return nullptr;
    free_render.disarm();
    return render;
}

C_API void render_destroy(Render* render)
{
    memfree(render->gpa, render, sizeof(Render) + render->vertex_capacity * sizeof(Vertex), alignof(Render));
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

C_API bool render_use_shader(Render* render, RenderShader shader)
{
    render_flush(render);

    FSFile* vert_file = fs_volume_use_ref(render->volume, shader.vert);
    if (fs_file_needs_reload(vert_file)) {
        if (!fs_file_reload(vert_file)) return false;
    }
    auto vert = fs_content(*vert_file);
    GLuint vs;
    if (!compile_shader_source(render->log, vert, GL_VERTEX_SHADER, &vs))
        return false;
    defer [&] { glDeleteShader(vs); };

    FSFile* frag_file = fs_volume_use_ref(render->volume, shader.frag);
    if (fs_file_needs_reload(frag_file)) {
        if (!fs_file_reload(frag_file)) return false;
    }
    auto frag = fs_content(*frag_file);
    GLuint fs;
    if (!compile_shader_source(render->log, frag, GL_FRAGMENT_SHADER, &fs))
        return false;
    defer [&] { glDeleteShader(fs); };

    auto program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    if (!link_program(render->log, program)) return false;

    glDeleteProgram(render->current_program);
    render->current_shaders = shader;
    render->current_program = program;
    glUseProgram(render->current_program);
    uniform_location(render->current_program, render->uniforms);
    glUniform1f(render->uniforms[UniformSlot_Time], render->time);
    glUniform2f(render->uniforms[UniformSlot_Resolution], render->resolution.x, render->resolution.y);
    glUniform2f(render->uniforms[UniformSlot_MousePosition], render->mouse_position.x, render->mouse_position.y);

    return true;
}

C_API void render_update(Render* render)
{
    auto* vert = fs_volume_use_ref(render->volume, render->current_shaders.vert);
    if (fs_file_needs_reload(vert)) {
        render->needs_reload = true;
        return;
    }
    auto* frag = fs_volume_use_ref(render->volume, render->current_shaders.frag);
    if (fs_file_needs_reload(frag)) {
        render->needs_reload = true;
        return;
    }
}

C_API bool render_needs_reload(Render const* render)
{
    return render->needs_reload;
}

C_API bool render_reload(Render* render)
{
    if (render_use_shader(render, render->current_shaders)) {
        render->needs_reload = false;
        return true;
    }
    return false;
}

C_API void render_flush(Render* render)
{
    if (render->vertex_index != 0) {
        glBufferSubData(GL_ARRAY_BUFFER,
                        0,
                        render->vertex_index * sizeof(Vertex),
                        render->vertices);
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
    if (render->vertex_capacity <= render->vertex_index + vertices) {
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
    case GL_GEOMETRY_SHADER:
        return "GL_GEOMETRY_SHADER";
    default:
        return "(Unknown)";
    }
}

static bool compile_shader_source(Logger* log, StringSlice source, GLenum shader_type, GLuint* out)
{
    GLuint shader = glCreateShader(shader_type);
    auto* data = (GLchar*)source.items;
    auto size = (GLint)source.count;
    glShaderSource(shader, 1, &data, &size);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLchar message[1024];
        GLsizei message_size = 0;
        glGetShaderInfoLog(shader, sizeof(message), &message_size, message);
        log->error("could not compile %s", shader_type_as_cstr(shader_type));
        log->error("%.*s", message_size, message);
        return false;
    }

    return *out = shader, true;
}

static bool link_program(Logger* log, GLuint program)
{
    glLinkProgram(program);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLsizei message_size = 0;
        GLchar message[1024];

        glGetProgramInfoLog(program, sizeof(message), &message_size, message);
        log->error("could not link: %.*s", message_size, message);
        return false;
    }

    return true;
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


static void uniform_location(GLuint program, GLint locations[UniformSlot__Count])
{
    for (int slot = 0; slot < UniformSlot__Count; ++slot) {
        locations[slot] = glGetUniformLocation(program, uniform_defs[slot].name);
    }
}

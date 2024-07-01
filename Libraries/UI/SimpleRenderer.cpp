#include "./SimpleRenderer.h"
#include <FS/Bundle.h>

#include <Ty/ScopeGuard.h>
#include <Ty/Defer.h>
#include <Ty/StringBuffer.h>

#include <Rexim/Util.h>
#include <Rexim/StringBuilder.h>
#include <Rexim/File.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define vert_shader_file_path "./Libraries/UI/Shaders/simple.vert"

namespace UI {

static_assert(COUNT_SIMPLE_SHADERS == 4, "The amount of fragment shaders has changed");
const char *frag_shader_file_paths[COUNT_SIMPLE_SHADERS] = {
    [SHADER_FOR_COLOR] = "./Libraries/UI/Shaders/simple_color.frag",
    [SHADER_FOR_IMAGE] = "./Libraries/UI/Shaders/simple_image.frag",
    [SHADER_FOR_TEXT] = "./Libraries/UI/Shaders/simple_text.frag",
    [SHADER_FOR_EPICNESS] = "./Libraries/UI/Shaders/simple_epic.frag",
};

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

static ErrorOr<GLuint> compile_shader_source(const GLchar *source, GLenum shader_type)
{
    auto shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLchar message[1024];
        GLsizei message_size = 0;
        glGetShaderInfoLog(shader, sizeof(message), &message_size, message);
        fprintf(stderr, "ERROR: could not compile %s\n", shader_type_as_cstr(shader_type));
        fprintf(stderr, "%.*s\n", message_size, message);
        return Error::from_string_literal("could not compile shader");
    }

    return shader;
}

static ErrorOr<GLuint> compile_shader_file(const char *file_path, GLenum shader_type)
{
    auto resource_path = StringView::from_c_string(file_path);
    auto resource = FS::Bundle::the().open(resource_path);
    if (!resource) {
        fprintf(stderr, "unknown resource: %s\n", file_path);
        return Error::from_string_literal("unknown shader");
    }
    auto buf = TRY(StringBuffer::create_fill(resource->view(), "\0"sv));
    return TRY(compile_shader_source(buf.data(), shader_type));
}

static void attach_shaders_to_program(GLuint *shaders, usize shaders_count, GLuint program)
{
    for (usize i = 0; i < shaders_count; ++i) {
        glAttachShader(program, shaders[i]);
    }
}

static ErrorOr<void> link_program(GLuint program, const char *file_path = __builtin_FILE(), usize line = __builtin_LINE())
{
    glLinkProgram(program);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLsizei message_size = 0;
        GLchar message[1024];

        glGetProgramInfoLog(program, sizeof(message), &message_size, message);
        fprintf(stderr, "%s:%zu: Program Linking: %.*s\n", file_path, line, message_size, message);
        return Error::from_string_literal("could not link program");
    }

    return {};
}

struct UniformDef {
    UniformSlot slot;
    const char *name;
};

static_assert(COUNT_UNIFORM_SLOTS == 4, "The amount of the shader uniforms have change. Please update the definition table accordingly");
static const UniformDef uniform_defs[COUNT_UNIFORM_SLOTS] = {
    [UNIFORM_SLOT_TIME] = {
        .slot = UNIFORM_SLOT_TIME,
        .name = "time",
    },
    [UNIFORM_SLOT_RESOLUTION] = {
        .slot = UNIFORM_SLOT_RESOLUTION,
        .name = "resolution",
    },
    [UNIFORM_SLOT_CAMERA_POS] = {
        .slot = UNIFORM_SLOT_CAMERA_POS,
        .name = "camera_pos",
    },
    [UNIFORM_SLOT_CAMERA_SCALE] = {
        .slot = UNIFORM_SLOT_CAMERA_SCALE,
        .name = "camera_scale",
    },
};


static void get_uniform_location(GLuint program, GLint locations[COUNT_UNIFORM_SLOTS])
{
    for (int slot = 0; slot < COUNT_UNIFORM_SLOTS; ++slot) {
        locations[slot] = glGetUniformLocation(program, uniform_defs[slot].name);
    }
}

ErrorOr<SimpleRenderer> SimpleRenderer::create()
{
    auto sr = SimpleRenderer();
    sr.m_verticies = (SimpleVertex*)calloc(sr.m_verticies_capacity, sizeof(SimpleVertex));
    if (!sr.m_verticies) {
        return Error::from_errno();
    }

    {
        glGenVertexArrays(1, &sr.m_vao);
        glBindVertexArray(sr.m_vao);

        glGenBuffers(1, &sr.m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, sr.m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sr.m_verticies_capacity * sizeof(SimpleVertex), sr.m_verticies, GL_DYNAMIC_DRAW);

        // position
        glEnableVertexAttribArray(SIMPLE_VERTEX_ATTR_POSITION);
        glVertexAttribPointer(
            SIMPLE_VERTEX_ATTR_POSITION,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(SimpleVertex),
            (GLvoid *) offsetof(SimpleVertex, position));

        // color
        glEnableVertexAttribArray(SIMPLE_VERTEX_ATTR_COLOR);
        glVertexAttribPointer(
            SIMPLE_VERTEX_ATTR_COLOR,
            4,
            GL_FLOAT,
            GL_FALSE,
            sizeof(SimpleVertex),
            (GLvoid *) offsetof(SimpleVertex, color));

        // uv
        glEnableVertexAttribArray(SIMPLE_VERTEX_ATTR_UV);
        glVertexAttribPointer(
            SIMPLE_VERTEX_ATTR_UV,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(SimpleVertex),
            (GLvoid *) offsetof(SimpleVertex, uv));
    }

    GLuint shaders[2] = {0};
    shaders[0] = TRY(compile_shader_file(vert_shader_file_path, GL_VERTEX_SHADER));
    Defer delete_first_shader = [&] {
        glDeleteShader(shaders[0]);
    };

    for (int i = 0; i < COUNT_SIMPLE_SHADERS; ++i) {
        shaders[1] = TRY(compile_shader_file(frag_shader_file_paths[i], GL_FRAGMENT_SHADER));
        Defer delete_shader = [&] {
            glDeleteShader(shaders[1]);
        };
        sr.m_programs[i] = glCreateProgram();
        attach_shaders_to_program(shaders, sizeof(shaders) / sizeof(shaders[0]), sr.m_programs[i]);
        TRY(link_program(sr.m_programs[i]));
    }
    return sr;
}

ErrorOr<void> SimpleRenderer::reload_shaders()
{
    GLuint programs[COUNT_SIMPLE_SHADERS];
    GLuint shaders[2] = {0};

    shaders[0] = TRY(compile_shader_file(vert_shader_file_path, GL_VERTEX_SHADER));
    Defer delete_shader = [&] {
        glDeleteShader(shaders[0]);
    };

    for (int i = 0; i < COUNT_SIMPLE_SHADERS; ++i) {
        shaders[1] = TRY(compile_shader_file(frag_shader_file_paths[i], GL_FRAGMENT_SHADER));
        Defer delete_shader = [&] {
            glDeleteShader(shaders[1]);
        };
        programs[i] = glCreateProgram();
        ScopeGuard delete_program = [&] {
            glDeleteProgram(programs[i]);
        };
        attach_shaders_to_program(shaders, sizeof(shaders) / sizeof(shaders[0]), programs[i]);
        TRY(link_program(programs[i]));
        delete_program.disarm();
    }

    for (int i = 0; i < COUNT_SIMPLE_SHADERS; ++i) {
        glDeleteProgram(m_programs[i]);
        m_programs[i] = programs[i];
    }
    printf("Reloaded shaders successfully!\n");

    return {};
}

// TODO: Don't render triples of verticies that form a triangle that is completely outside of the screen
//
// Ideas on how to check if a triangle is outside of the screen:
// 1. Apply camera transformations to the triangle.
// 2. Form an axis-aligned boundary box (AABB) of the triangle.
// 3. Check if the Triangle AABB does not intersect the Screen AABB.
//
// This might not be what we want at the end of the day, though. Because in case of a lot of triangles we
// end up iterating each of them at least once and doing camera trasformations on the CPU (which is
// something we do on GPU already).
//
// It would be probably better if such culling occurred on a higher level of abstractions. For example
// in the Editor. For instance, if the Editor noticed that the line it is currently rendering is
// below the screen, it should stop rendering the rest of the text, thus never calling
// vertex() for a potentially large amount of verticies in the first place.
void SimpleRenderer::vertex(Vec2f p, Vec4f c, Vec2f uv)
{
#if 0
    // TODO: flush the renderer on vertex buffer overflow instead firing the assert
    if (sr->verticies_count >= SIMPLE_VERTICIES_CAP) simple_renderer_flush(sr);
#else
    // NOTE: it is better to just crash the app in this case until the culling described
    // above is sorted out.
    assert(m_verticies_count < SIMPLE_VERTICIES_CAP);
#endif
    SimpleVertex *last = &m_verticies[m_verticies_count];
    last->position = p;
    last->color    = c;
    last->uv       = uv;
    m_verticies_count += 1;
}

void SimpleRenderer::triangle(Vec2f p0, Vec2f p1, Vec2f p2,
                              Vec4f c0, Vec4f c1, Vec4f c2,
                              Vec2f uv0, Vec2f uv1, Vec2f uv2)
{
    vertex(p0, c0, uv0);
    vertex(p1, c1, uv1);
    vertex(p2, c2, uv2);
}

// 2-3
// |\|
// 0-1
void SimpleRenderer::quad(Vec2f p0, Vec2f p1, Vec2f p2, Vec2f p3,
                          Vec4f c0, Vec4f c1, Vec4f c2, Vec4f c3,
                          Vec2f uv0, Vec2f uv1, Vec2f uv2, Vec2f uv3)
{
    triangle(p0, p1, p2, c0, c1, c2, uv0, uv1, uv2);
    triangle(p1, p2, p3, c1, c2, c3, uv1, uv2, uv3);
}

void SimpleRenderer::image_rect(Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, Vec4f c)
{
    quad(
        p, p + vec2f(s.x, 0), p + vec2f(0, s.y), p + s,
        c, c, c, c,
        uvp, uvp + vec2f(uvs.x, 0), uvp + vec2f(0, uvs.y), uvp + uvs);
}

void SimpleRenderer::solid_rect(Vec2f p, Vec2f s, Vec4f c)
{
    Vec2f uv = vec2fs(0);
    quad(
        p, p + vec2f(s.x, 0), p + vec2f(0, s.y), p + s,
        c, c, c, c,
        uv, uv, uv, uv);
}

void SimpleRenderer::outline_rect(Vec2f point, Vec2f size, f32 outline_width, Vec4f fill_color, Vec4f outline_color)
{
    Vec2f left = point;
    Vec2f left_size = vec2f(outline_width, size.y);

    Vec2f top = point + vec2f(0.0f, size.y) - vec2f(0.0f, outline_width);
    Vec2f top_size = vec2f(size.x, outline_width);

    Vec2f right = point + vec2f(size.x, 0.0f) - vec2f(outline_width, 0.0f);
    Vec2f right_size = vec2f(outline_width, size.y);

    Vec2f bottom = point;
    Vec2f bottom_size = vec2f(size.x, outline_width);

    Vec2f fill = point + vec2fs(outline_width);
    Vec2f fill_size = size - vec2fs(2.0f * outline_width);

    solid_rect(left,   left_size,   outline_color);
    solid_rect(right,  right_size,  outline_color);
    solid_rect(top,    top_size,    outline_color);
    solid_rect(bottom, bottom_size, outline_color);
    solid_rect(fill,   fill_size, fill_color);
}

void SimpleRenderer::outline_rect(OutlineRect const& args)
{
    auto [point, size, outline_width, fill_color, left_color, top_color, right_color, bottom_color] = args;

    if (left_color.a == 0.0f) {
        left_color = fill_color;
    }
    if (top_color.a == 0.0f) {
        top_color = fill_color;
    }
    if (right_color.a == 0.0f) {
        right_color = fill_color;
    }
    if (bottom_color.a == 0.0f) {
        bottom_color = fill_color;
    }

    Vec2f left = args.point;
    Vec2f left_size = vec2f(outline_width, size.y);

    Vec2f top = point + vec2f(0.0f, size.y) - vec2f(0.0f, outline_width);
    Vec2f top_size = vec2f(size.x, outline_width);

    Vec2f right = point + vec2f(size.x, 0.0f) - vec2f(outline_width, 0.0f);
    Vec2f right_size = vec2f(outline_width, size.y);

    Vec2f bottom = point;
    Vec2f bottom_size = vec2f(size.x, outline_width);

    Vec2f fill = point + vec2fs(outline_width);
    Vec2f fill_size = size - vec2fs(2.0f * outline_width);

    solid_rect(top,    top_size,    top_color);
    solid_rect(bottom, bottom_size, bottom_color);
    solid_rect(left,   left_size,   left_color);
    solid_rect(right,  right_size,  right_color);
    solid_rect(fill,   fill_size, fill_color);
}

void SimpleRenderer::sync()
{
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    m_verticies_count * sizeof(SimpleVertex),
                    m_verticies);
}

void SimpleRenderer::draw()
{
    glDrawArrays(GL_TRIANGLES, 0, m_verticies_count);
}

void SimpleRenderer::set_shader(SimpleShader shader)
{
    if (m_current_shader == shader)
        return;
    flush();
    m_current_shader = shader;
    glUseProgram(m_programs[m_current_shader]);
    get_uniform_location(m_programs[m_current_shader], m_uniforms);
    glUniform2f(m_uniforms[UNIFORM_SLOT_RESOLUTION], m_resolution.x, m_resolution.y);
    glUniform1f(m_uniforms[UNIFORM_SLOT_TIME], m_time);
    glUniform2f(m_uniforms[UNIFORM_SLOT_CAMERA_POS], m_camera_pos.x, m_camera_pos.y);
    glUniform1f(m_uniforms[UNIFORM_SLOT_CAMERA_SCALE], m_camera_scale);
}

void SimpleRenderer::flush()
{
    if (m_verticies_count != 0) {
        sync();
        draw();
        m_verticies_count = 0;
    }
}

}

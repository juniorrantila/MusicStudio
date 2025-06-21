#include "./Renderer.h"
#include "GL.h"

#include <Ty2/Defer.h>
#include <Ty2/Verify.h>
#include <Ty2/Hash.h>

#include <stdio.h>

static constexpr GLint gl_uniform_location_null = -1;
static constexpr GLuint gl_program_null = -1;

static bool render_init(GLRenderer*);
static bool compile_shader(Logger* log, GLenum shader_type, char const* source, u32 source_len, GLuint* out);
static bool link_program(Logger* log, GLuint vert, GLuint frag, GLuint* out);
static void use_shader(GLRenderer*, GLShaderID);

void GLRenderer::reserve(u32 count) { return gl_reserve(this, count); }
void gl_reserve(GLRenderer* r, u32 count)
{
    if (r->vertex_count + count >= gl_vertex_count_max) {
        gl_flush(r);
        VERIFY(r->vertex_count == 0);
    }
}

void GLRenderer::push_quad(GLShaderID shader, GLQuad q) { return gl_push_quad(this, shader, q); }
void gl_push_quad(GLRenderer* r, GLShaderID shader, GLQuad q)
{
    gl_reserve(r, 6);
    gl_push_vert(r, shader, q.p0);
    gl_push_vert(r, shader, q.p1);
    gl_push_vert(r, shader, q.p2);
    gl_push_vert(r, shader, q.p1);
    gl_push_vert(r, shader, q.p2);
    gl_push_vert(r, shader, q.p3);
}

void GLRenderer::push_triangle(GLShaderID shader, GLTriangle t) { return gl_push_triangle(this, shader, t); }
void gl_push_triangle(GLRenderer* r, GLShaderID shader, GLTriangle t)
{
    gl_reserve(r, 3);
    gl_push_vert(r, shader, t.p0);
    gl_push_vert(r, shader, t.p1);
    gl_push_vert(r, shader, t.p2);
}

void GLRenderer::push_vert(GLShaderID shader, GLVertex vert) { return gl_push_vert(this, shader, vert); }
void gl_push_vert(GLRenderer* r, GLShaderID shader, GLVertex vert)
{
    if (r->current_shader.id != shader.id && shader.id != gl_shader_id_null.id)
        use_shader(r, shader);

    gl_reserve(r, 1);
    r->vertexes[r->vertex_count++] = vert;
}

void GLRenderer::uniform1f(GLUniformID uniform, f32 value) { return gl_uniform1f(this, uniform, value); }
void gl_uniform1f(GLRenderer* r, GLUniformID uniform, f32 value) {
    r->uniforms[uniform.id].count = 1;
    r->uniforms[uniform.id].values[0] = value;

    auto* program = &r->shaders[r->current_shader.id];

    auto location = program->uniform_locations[uniform.id];
    if (location == gl_uniform_location_null) return;

    glProgramUniform1f(program->program, location, value);
}

void GLRenderer::uniform2f(GLUniformID uniform, v2 v) { return gl_uniform2f(this, uniform, v); }
void gl_uniform2f(GLRenderer* r, GLUniformID uniform, v2 v)
{
    r->uniforms[uniform.id].count = 2;
    r->uniforms[uniform.id].values[0] = v.x;
    r->uniforms[uniform.id].values[1] = v.y;

    auto program = r->shaders[r->current_shader.id];
    auto slot = program.uniform_locations[uniform.id];
    if (slot == gl_uniform_location_null) return;

    glProgramUniform2f(program.program, slot, v.x, v.y);
}

void GLRenderer::uniform4f(GLUniformID uniform, v4 v) { return gl_uniform4f(this, uniform, v); }
void gl_uniform4f(GLRenderer* r, GLUniformID uniform, v4 v)
{
    r->uniforms[uniform.id].count = 4;
    r->uniforms[uniform.id].values[0] = v.x;
    r->uniforms[uniform.id].values[1] = v.y;
    r->uniforms[uniform.id].values[2] = v.z;
    r->uniforms[uniform.id].values[3] = v.w;

    auto program = r->shaders[r->current_shader.id];
    auto slot = program.uniform_locations[uniform.id];
    if (slot == gl_uniform_location_null) return;

    glProgramUniform4f(program.program, slot, v.x, v.y, v.z, v.w);
}

void GLRenderer::flush() { return gl_flush(this); }
void gl_flush(GLRenderer* r)
{
    if (r->vertex_count == 0)
        return;

    glBindVertexArray(r->vao);
    Defer unbind_vao = []{
        glBindVertexArray(0);
    };

    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    Defer unbind_vbo = []{
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    };

    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    (GLsizeiptr)(r->vertex_count * sizeof(GLVertex)),
                    r->vertexes);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)r->vertex_count);

    r->vertex_count = 0;
}

void GLRenderer::clear(v4 color) { return gl_clear(this, color); }
void gl_clear(GLRenderer*, v4 color)
{
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

GLShaderID GLRenderer::shader(Logger* log, GLShaderSource source) { return gl_shader(this, log, source); }
GLShaderID gl_shader(GLRenderer* r, Logger* log, GLShaderSource source)
{
    if (!render_init(r))
        return gl_shader_id_null;

    u64 vert_full_hash = djb2(djb2_initial_seed, source.vert.items, source.vert.count);
    u64 frag_full_hash = djb2(djb2_initial_seed, source.frag.items, source.frag.count);
    u16 slot = djb2_u64(vert_full_hash, frag_full_hash) % gl_shader_count_max;
    VERIFY(slot != gl_shader_id_null.id);

    u16 vert_hash = (u16)vert_full_hash;
    u16 frag_hash = (u16)frag_full_hash;

    auto* shader = &r->shaders[slot];
    if (shader->vert_source_hash == vert_hash && shader->frag_source_hash == frag_hash) {
        if (shader->program == gl_program_null)
            return gl_shader_id_null;
        return (GLShaderID){ .id = slot };
    }

    if (shader->vert_source_hash != vert_hash | shader->frag_source_hash != frag_hash) {
        if (shader->vert_source_hash != 0 | shader->frag_source_hash != 0) {
            if (shader->program != gl_program_null) glDeleteProgram(shader->program);
        }
    }
    *shader = (GLShader){
        .program = gl_program_null,
        .vert_source_hash = vert_hash,
        .frag_source_hash = frag_hash,
        .uniform_locations = {},
    };

    GLuint vertex;
    if (!compile_shader(log, GL_VERTEX_SHADER, source.vert.items, source.vert.count, &vertex))
        return gl_shader_id_null;
    defer [&] { glDeleteShader(vertex); };

    GLuint fragment;
    if (!compile_shader(log, GL_FRAGMENT_SHADER, source.frag.items, source.frag.count, &fragment))
        return gl_shader_id_null;
    defer [&] { glDeleteShader(fragment); };

    if (!link_program(log, vertex, fragment, &shader->program))
        return gl_shader_id_null;

    log->info("loaded shader");
    return (GLShaderID){ .id = slot };
}

GLUniformID GLRenderer::uniform(c_string name) { return gl_uniform(this, name); }
GLUniformID gl_uniform(GLRenderer* r, c_string name)
{
    if (!render_init(r))
        return gl_uniform_id_null;

    u32 len = __builtin_strlen(name);
    VERIFY(len <= gl_uniform_name_max);
    alignas(256) u8 n[gl_uniform_name_max] = {};
    __builtin_memcpy(n, name, len);
    for (u16 i = 0; i < r->uniform_count; i++) {
        if (__builtin_memcmp(n, r->uniforms[i].name, gl_uniform_name_max) == 0) {
            return (GLUniformID){.id = i};
        }
    }
    u16 id = r->uniform_count++;
    VERIFY(id < gl_uniform_count_max);

    r->uniforms[id] = {};
    __builtin_memcpy(r->uniforms[id].name, n, gl_uniform_name_max);

    return (GLUniformID){.id = id};
}

static bool render_init(GLRenderer* r)
{
    if (r->initialized)
        return true;

    glGenVertexArrays(1, &r->vao);
    glBindVertexArray(r->vao);
    defer []{ glBindVertexArray(0); };

    glGenBuffers(1, &r->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    defer []{ glBindBuffer(GL_ARRAY_BUFFER, 0); };
    glBufferData(GL_ARRAY_BUFFER, gl_vertex_count_max * sizeof(GLVertex), r->vertexes, GL_DYNAMIC_DRAW);

    for (u32 i = 0; i < ty_array_size(gl_vertex_attribs); i++) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, gl_vertex_attribs[i].count, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void*)gl_vertex_attribs[i].offset);
    }

    r->initialized = true;
    return true;
}

static bool compile_shader(Logger* log, GLenum shader_type, char const* source, u32 source_len, GLuint* out)
{
    GLuint shader = glCreateShader(shader_type);
    auto* data = (GLchar*)source;
    auto size = (GLint)source_len;
    glShaderSource(shader, 1, &data, &size);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLchar message[1024];
        GLsizei message_size = 0;
        glGetShaderInfoLog(shader, sizeof(message), &message_size, message);
        log->error("could not compile shader: %.*s", message_size - 1, message);
        return false;
    }

    return *out = shader, true;
}

static bool link_program(Logger* log, GLuint vert, GLuint frag, GLuint* out)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        glDeleteProgram(program);
        GLsizei message_size = 0;
        GLchar message[1024];

        glGetProgramInfoLog(program, sizeof(message), &message_size, message);
        log->error("could not link: %.*s", message_size, message);
        return false;
    }

    return *out = program, true;
}

static void use_shader(GLRenderer* r, GLShaderID shader)
{
    if (r->current_shader.id == shader.id)
        return;
    if (r->current_shader.id == gl_shader_id_null.id)
        return;

    gl_flush(r);
    auto* program = &r->shaders[shader.id];
    glUseProgram(program->program);
    for (u32 i = 0; i < gl_uniform_count_max; i++) {
        auto* uniform = &r->uniforms[i];
        if (r->uniforms[i].count == 0) continue;
        GLint location = glGetUniformLocation(program->program, uniform->name);
        if (location == gl_uniform_location_null) continue;
        program->uniform_locations[i] = location;

        switch (uniform->count) {
        case 1:
            glProgramUniform1fv(program->program, program->uniform_locations[i], 1, uniform->values);
            break;
        case 2:
            glProgramUniform2fv(program->program, program->uniform_locations[i], 1, uniform->values);
            break;
        case 4:
            glProgramUniform4fv(program->program, program->uniform_locations[i], 1, uniform->values);
            break;
        default:
            UNREACHABLE();
        }
    }

    r->current_shader = shader;
}

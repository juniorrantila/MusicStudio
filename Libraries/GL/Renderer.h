#pragma once
#include <Ty2/Base.h>
#include <Ty/StringSlice.h>
#include <Ty2/Logger.h>

#include "./GL.h"

typedef struct GLVertexAttrib {
    GLint count;
    u64 offset;
} GLVertexAttrib;

constexpr u32 gl_vertex_count_max = 8 * 1024;
typedef struct GLVertex {
    v4 color;
    v4 bits;
    v2 position;
    v2 uv0;
    v2 uv1;
} GLVertex;
static_assert(sizeof(GLVertex) == 64);
static constexpr GLVertexAttrib gl_vertex_attribs[] = {
    (GLVertexAttrib){
        .count = 4,
        .offset = __builtin_offsetof(GLVertex, color),
    },
    (GLVertexAttrib){
        .count = 4,
        .offset = __builtin_offsetof(GLVertex, bits),
    },
    (GLVertexAttrib){
        .count = 2,
        .offset = __builtin_offsetof(GLVertex, position),
    },
    (GLVertexAttrib){
        .count = 2,
        .offset = __builtin_offsetof(GLVertex, uv0),
    },
    (GLVertexAttrib){
        .count = 2,
        .offset = __builtin_offsetof(GLVertex, uv1),
    },
};

//* 2
//* |\
//* 0-1
typedef struct GLTriangle {
    GLVertex p0;
    GLVertex p1;
    GLVertex p2;
} GLTriangle;

constexpr u32 gl_uniform_name_max = 14;
constexpr u32 gl_uniform_count_max = 32;
typedef struct { u16 id; } GLUniformID;
typedef struct GLUniform {
    f32 values[4];
    u8 count;

    char name[gl_uniform_name_max];
    u8 zero;
} GLUniform;
static_assert(sizeof(GLUniform) == 32);
constexpr GLUniformID gl_uniform_id_null = {0xFFFF};


constexpr u32 gl_shader_count_max = 1024;
typedef struct GLShaderID { u16 id; } GLShaderID;
typedef struct GLShader {
    GLuint program;
    u16 vert_source_hash;
    u16 frag_source_hash;

    GLint uniform_locations[gl_uniform_count_max];
} GLShader;
static_assert(sizeof(GLShader) == 136);
constexpr GLShaderID gl_shader_id_null = {0xFFFF};

//* 2-3
//* |\|
//* 0-1
typedef struct GLQuad {
    GLVertex p0;
    GLVertex p1;
    GLVertex p2;
    GLVertex p3;
} GLQuad;

typedef struct GLShaderSource {
    StringSlice vert;
    StringSlice frag;
} GLShaderSource;

typedef struct GLRenderer {
    u32 initialized;

    u32 vertex_count;
    u16 uniform_count;
    GLShaderID current_shader;

    GLuint vao;
    GLuint vbo;

    GLUniform uniforms[gl_uniform_count_max];
    GLShader shaders[gl_shader_count_max];
    GLVertex vertexes[gl_vertex_count_max];

#ifdef __cplusplus
    void clear(v4 color);

    GLShaderID shader(Logger* log, GLShaderSource);
    GLUniformID uniform(c_string name);

    void uniform1f(GLUniformID, f32);
    void uniform2f(GLUniformID, v2);
    void uniform4f(GLUniformID, v4);

    void reserve(u32);

    //* 2-3
    //* |\|
    //* 0-1
    void push_quad(GLShaderID, GLQuad);
    void push_triangle(GLShaderID, GLTriangle);
    void push_vert(GLShaderID, GLVertex);

    void flush();
#endif
} GLRenderer;
static_assert(sizeof(GLRenderer) == 664608);

C_API void gl_clear(GLRenderer*, v4 color);

C_API GLShaderID gl_shader(GLRenderer*, Logger*, GLShaderSource source);
C_API GLUniformID gl_uniform(GLRenderer*, c_string name);

C_API void gl_uniform1f(GLRenderer*, GLUniformID, f32);
C_API void gl_uniform2f(GLRenderer*, GLUniformID, v2);
C_API void gl_uniform4f(GLRenderer*, GLUniformID, v4);

C_API void gl_flush(GLRenderer*);
C_API void gl_reserve(GLRenderer*, u32 count);


//* 2-3
//* |\|
//* 0-1
C_API void gl_push_quad(GLRenderer*, GLShaderID, GLQuad);
C_API void gl_push_triangle(GLRenderer*, GLShaderID, GLTriangle);
C_API void gl_push_vert(GLRenderer*, GLShaderID, GLVertex);

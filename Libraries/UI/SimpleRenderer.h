#pragma once
#include "./Graphics/GL.h"

#include <Ty/ErrorOr.h>
#include <FS/Bundle.h>

#include <Rexim/LA.h>

namespace UI {

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
    static ErrorOr<SimpleRenderer> create(FS::Bundle& bundle);

    void set_shader(SimpleShader shader);

    constexpr Vec2f resolution() const { return m_resolution; }
    constexpr void set_resolution(Vec2f resolution) { m_resolution = resolution; }

    constexpr f32 time() const { return m_time; }
    constexpr void set_time(f32 time) { m_time = time; }

    constexpr f32 camera_scale() const { return m_camera_scale; }
    constexpr void set_camera_scale(f32 scale) { m_camera_scale = scale; }

    constexpr Vec2f camera_pos() const { return m_camera_pos; }
    constexpr void set_camera_pos(Vec2f pos) { m_camera_pos = pos; }

    constexpr Vec2f camera_vel() const { return m_camera_vel; }
    constexpr void set_camera_vel(Vec2f vel) { m_camera_vel = vel; }

    constexpr f32 camera_scale_vel() const { return m_camera_scale_vel; }
    constexpr void set_camera_scale_vel(f32 vel) { m_camera_scale_vel = vel; }

    ErrorOr<void> reload_shaders();

    void outline_rect(Vec2f p, Vec2f size, f32 outline_size, Vec4f fill_color, Vec4f outline_color);

    struct OutlineRect {
        Vec2f point { vec2fs(0.0f) };
        Vec2f size { vec2fs(0.0f) };
        f32 outline_size { 0.0f };
        Vec4f fill_color { vec4fs(0) };
        Vec4f left_color { vec4fs(0) };
        Vec4f top_color { vec4fs(0) };
        Vec4f right_color { vec4fs(0) };
        Vec4f bottom_color { vec4fs(0) };
    };
    void outline_rect(OutlineRect const&);

    void solid_rect(Vec2f p, Vec2f s, Vec4f c);
    void image_rect(Vec2f p, Vec2f s, Vec2f uvp, Vec2f uvs, Vec4f c);

    void flush();

private:
    SimpleRenderer(FS::Bundle& bundle)
        : m_bundle(bundle)
    {
    }

    void draw();
    void sync();
    void vertex(Vec2f p, Vec4f c, Vec2f uv);
    void triangle(Vec2f p0, Vec2f p1, Vec2f p2,
                  Vec4f c0, Vec4f c1, Vec4f c2,
                  Vec2f uv0, Vec2f uv1, Vec2f uv2);
    void quad(Vec2f p0, Vec2f p1, Vec2f p2, Vec2f p3,
              Vec4f c0, Vec4f c1, Vec4f c2, Vec4f c3,
              Vec2f uv0, Vec2f uv1, Vec2f uv2, Vec2f uv3);

    FS::Bundle& m_bundle;

    GLuint m_vao { 0 };
    GLuint m_vbo { 0 };
    GLuint m_programs[COUNT_SIMPLE_SHADERS] {};
    SimpleShader m_current_shader { (SimpleShader)-1 };

    GLint m_uniforms[COUNT_UNIFORM_SLOTS] {};

    SimpleVertex* m_verticies { nullptr };
    usize m_verticies_capacity { SIMPLE_VERTICIES_CAP };
    usize m_verticies_count { 0 };

    Vec2f m_resolution { 0.0f, 0.0f };
    f32 m_time { 0.0f };

    Vec2f m_camera_pos { 0.0f, 0.0f };
    f32 m_camera_scale { 1.0f };
    f32 m_camera_scale_vel { 0.0f };
    Vec2f m_camera_vel { 0.0f, 0.0f };
};


}

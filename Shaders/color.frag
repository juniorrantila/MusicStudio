#version 330 core

uniform vec2 resolution;
uniform vec2 mouse_position;
uniform sampler2D image;

out vec4 frag_color;

in vec4 out_color;
in vec2 out_uv;
flat in uint out_flags;

const uint KIND_COLOR  = 0u;
const uint KIND_IMAGE  = 1u;
const uint KIND_TEXT   = 2u;
const uint KIND_CURSOR = 3u;

uint kind(uint flags);
void color_shader();
void image_shader();
void text_shader();
void cursor_shader();
void unreachable();

void main() {
    switch (kind(out_flags)) {
    case KIND_COLOR:
        color_shader();
        break;
    case KIND_IMAGE:
        image_shader();
        break;
    case KIND_TEXT:
        text_shader();
        break;
    case KIND_CURSOR:
        cursor_shader();
        break;
    default:
        unreachable();
        break;
    }
}

void unreachable()
{
    frag_color = vec2(1, 0).xyxx;
    float invalid = 1.0 / 0.0;
    while(true) {}
}

uint kind(uint flags) {
    return flags & 0x0Fu;
}

void color_shader()
{
    frag_color = out_color;
}

void image_shader()
{
    frag_color = texture(image, out_uv);
}

void text_shader()
{
    float d = texture(image, out_uv).r;
    float aaf = fwidth(d);
    float alpha = smoothstep(0.5 - aaf, 0.5 + aaf, d);
    frag_color = vec4(out_color.rgb, alpha);
}

void cursor_shader()
{
    vec2 f = gl_FragCoord.xy;
    f.y = resolution.y - gl_FragCoord.y;
    float d = length(f / 2 - mouse_position);
    frag_color = vec2(d < 10).xxxx * out_color;
}

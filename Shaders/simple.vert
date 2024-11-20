#version 330 core

uniform vec2 resolution;
uniform float time;

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;

out vec4 out_color;
out vec2 out_uv;

vec2 screen_space_from_top_left(vec2 position)
{
    vec2 p = (position * 2) - 1;
    return vec2(p.x, -1 * p.y);
}

void main() {
    gl_Position = vec4(screen_space_from_top_left(position), 0, 1);
    out_color = color;
    out_uv = uv;
}

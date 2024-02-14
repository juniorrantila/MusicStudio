#version 330 core

out vec4 frag_color;
uniform sampler2D image;

in vec2 out_uv;

void main() {
    frag_color = texture(image, out_uv);
}

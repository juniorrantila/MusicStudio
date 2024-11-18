#version 330 core

out vec4 frag_color;
uniform sampler2D image;

in vec4 out_color;
in vec2 out_uv;

void main() {
    float d = texture(image, out_uv).r;
    float aaf = fwidth(d);
    float alpha = smoothstep(0.5 - aaf, 0.5 + aaf, d);
    frag_color = vec4(out_color.rgb, alpha);
}

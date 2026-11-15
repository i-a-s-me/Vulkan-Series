#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_texcord;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec2 out_texcoord;

void main() {
    gl_Position = vec4(in_pos, 1.0);
    out_color = in_color;
    out_texcoord = in_texcord;
}

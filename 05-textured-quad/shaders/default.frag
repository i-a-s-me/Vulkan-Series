#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec2 in_texcoord;

layout(location = 0) out vec4 finalColor;

void main() {
    finalColor = vec4(in_texcoord, 1.0, 1.0);
}

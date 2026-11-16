#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec2 in_texcoord;

layout(set = 1, binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) out vec4 finalColor;

void main() {
    finalColor = texture(texture_sampler, in_texcoord);
}

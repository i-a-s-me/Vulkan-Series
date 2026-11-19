#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_texcoord;

layout(std140, set = 0, binding = 0) uniform VertexUniformData {
    mat4 model;
    mat4 view;
    mat4 projection;
} uniform_data;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec2 out_texcoord;

void main() {
    gl_Position = uniform_data.projection * uniform_data.view * uniform_data.model * vec4(in_pos, 1.0);
    out_color = in_color;
    out_texcoord = in_texcoord;
}

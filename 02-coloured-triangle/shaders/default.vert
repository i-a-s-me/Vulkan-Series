#version 450

struct VertexData {
    vec2 pos;
    vec3 color;
};

VertexData vertices[3] = VertexData[](
    VertexData(vec2(0.0, -0.5), vec3(1.0, 0.0, 0.0)),
    VertexData(vec2(0.5, 0.5), vec3(0.0, 1.0, 0.0)),
    VertexData(vec2(-0.5, 0.5), vec3(0.0, 0.0, 1.0))
);

layout(location = 0) out vec3 out_color;

void main() {
    VertexData vertex = vertices[gl_VertexIndex];

    out_color = vertex.color;
    gl_Position = vec4(vertex.pos, 0.0, 1.0);
}

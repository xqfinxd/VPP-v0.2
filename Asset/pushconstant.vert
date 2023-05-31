// Vertex shader
#version 450

layout (push_constant) uniform PushConsts {
    mat4 mvp;
} pushConsts;

layout (location = 0) in mat2 inPos;
layout (location = 3) in vec2 inColor;

layout (location = 0) out vec3 outColor;

void main() {
    gl_Position = pushConsts.mvp * vec4(inPos[0], 1.0, 1.0);
    outColor = vec3(inColor, 1.0);
}
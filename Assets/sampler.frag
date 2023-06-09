#version 450

layout(set = 0, binding = 0) uniform texture2D tex;
layout(set = 1, binding = 0) uniform sampler samp;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outFragColor;

void main() {
    outFragColor = texture(sampler2D(tex,samp), inUV);
}
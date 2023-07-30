#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoord;

layout (set=1, binding=0) uniform sampler2D texture1;
layout (set=1, binding=1) uniform sampler2D texture2;

void main()
{
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.8);
} 
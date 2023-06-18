#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 FragColor;

void main()
{
    FragColor = vec4(0.0f, 1.f, 0.2f, 1.0f);
} 
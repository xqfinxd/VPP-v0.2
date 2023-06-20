#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 FragColor;
layout (location = 0) in vec3 ourColor;

void main()
{
    FragColor = vec4(ourColor, 1.0f);
} 
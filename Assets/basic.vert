#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_enhanced_layouts : enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

layout (location = 0) out vec3 ourColor;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y + gl_InstanceIndex, aPos.z, 1.0);
	ourColor = aColor;
}
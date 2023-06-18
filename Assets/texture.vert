#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_enhanced_layouts : enable

layout (std140, set = 0, binding = 0) uniform MVP {
    mat4 model;
	mat4 view;
    mat4 perpective;
} mvp;
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2, component=0) in float uv_x;
layout (location = 2, component=1) in float uv_y;
layout (location = 0) out vec2 texcoord;
void main() {
	mat4 mat = mvp.perpective * mvp.view * mvp.model;
	texcoord = vec2(uv_x, 1.f - uv_y);
	gl_Position = mat * vec4(position, 1.0f);
}

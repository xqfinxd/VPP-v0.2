#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (std140, set = 0, binding = 0) uniform MVP {
    mat4 model;
	mat4 view;
    mat4 perpective;
} mvp;
layout (std140, set = 2, binding = 0) uniform Light {
	vec4 color;
	vec3 direct;
	float intensity;

} light;
layout (std140, set = 3, binding = 0) uniform Camera {
	vec3 position;
	vec3 forward;
	float fovy;
	float aspect;
	float near;
	float far;
} camera;
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 uv;
layout (location = 0) out vec2 texcoord;
layout (location = 1) out vec4 lightColor;
void main() {
	mat4 mat = mvp.perpective * mvp.view * mvp.model;
	gl_Position = mat * vec4(position, 1.0f);
	texcoord = vec2(uv.x, 1.f - uv.y);
}

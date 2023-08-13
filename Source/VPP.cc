#include "VPP.hpp"
#include "VPP_Core.h"

#include "device/GraphicsDevice.h"
#include "GlslParser.h"

namespace VPP {

static GraphicsDevice* g_Device = nullptr;

GraphicsDevice* GetDevice() {
  return g_Device;
}

const char* basicVert = R"(
#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_enhanced_layouts : enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

layout (location = 0) out vec2 TexCoord;

layout (std140, set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
} mvp;

layout (push_constant) uniform PushConsts {
    vec4 offset;
    float alpha;
} pushConsts;

void main()
{
    gl_Position = mvp.projection * mvp.view * mvp.model * vec4(aPos, pushConsts.alpha) + pushConsts.offset;
	  TexCoord = vec2(aTexCoord.x, aTexCoord.y);
}
)";

const char* basicFrag = R"(
#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoord;

layout (set=1, binding=1) uniform sampler2D texture1;
layout (set=1, binding=2) uniform sampler2D texture2;

void main()
{
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.8);
} 
)";


void InitDevice(SDL_Window* window) {
  if (g_Device) return;
  g_Device = new GraphicsDevice(window);

  std::vector<GlslSource> sources(2);
  sources[0] = {basicVert, "main", vk::ShaderStageFlagBits::eVertex};
  sources[1] = {basicFrag, "main", vk::ShaderStageFlagBits::eFragment};

  GlslParser parser;
  if (parser.Load(sources)) {
    auto uniforms = parser.GetUniforms();
    printf("%d\n", (int)uniforms.size());

    auto attribs = parser.GetVertexAttributes();
    printf("%d\n", (int)attribs.size());

    auto pushes = parser.GetPushConstants();
    printf("%d\n", (int)pushes.size());
  }
}

void QuitDevice() {
  if (!g_Device) return;
  delete g_Device;
}

} // namespace VPP
#pragma once

#include <vector>
#include <string>
#include <map>

#ifdef GLSLPARSER_EXPORTS
#define SHADER_API __declspec(dllexport)
#else
#define SHADER_API __declspec(dllimport)
#endif // SHADER_EXPORTS

#include <vulkan/vulkan.hpp>

// clang-format off

class GlslParserImpl;

struct GlslBlockMember {
  std::string m_Name;
  uint32_t    m_Offset;
};

struct GlslUniform {
  vk::DescriptorType            m_Type;
  vk::ShaderStageFlags          m_Stages;
  std::string                   m_Name;
  uint32_t                      m_Set;
  uint32_t                      m_Binding;
  uint32_t                      m_Size;       // only buffer
  std::vector<GlslBlockMember>  m_Members;    // only block
};

struct GlslVertexAttribute {
  uint32_t              m_Location;
  vk::ShaderStageFlags  m_Stages;
  uint32_t              m_Size;
  std::string           m_Name;
};

struct GlslPushConstant {
  uint32_t                      m_Size;
  vk::ShaderStageFlags          m_Stages;
  std::string                   m_Name;
  std::vector<GlslBlockMember>  m_Members;    // only block
};

struct GlslSource {
  std::string             m_Content;
  std::string             m_Entry;
  vk::ShaderStageFlagBits m_Stage;
};

class SHADER_API GlslParser {
public:
  GlslParser();
  ~GlslParser();

  bool Load(const std::vector<GlslSource>& contents);
  std::vector<GlslUniform> GetUniforms() const;
  std::vector<GlslVertexAttribute> GetVertexAttributes() const;
  std::vector<GlslPushConstant> GetPushConstants() const;

private:
  GlslParserImpl* m_Impl;
};

// clang-format on

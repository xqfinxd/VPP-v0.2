#pragma once

#include "Shader.h"

#include <vulkan/vulkan.hpp>

namespace VPP {

class ShaderImpl {
public:
  struct Source {
    std::string m_Content;
    std::string m_Entry;
  };

  enum BufferValue {
    eNone,
    eMVP,
    eTextureST,
  };

  struct BindBuffer {
    uint32_t      m_Set;
    uint32_t      m_Binding;
    BufferValue m_Value;
  };

  enum VertexValue {
    eNone,
    eVertices,
    eNormals,
    eTangents,
    eColors,
    eUVs,
    eUVs2,
    eUVs3,
    eUVs4,
    eUVs5,
    eUVs6,
    eUVs7,
    eUVs8,
  };

  struct BindVertex {
    uint32_t    m_Location;
    VertexValue m_Value;
  };

  struct TextureColor {
    uint32_t m_Location;
    VertexValue m_Value;
  };

  struct Pass {
    using StageSources = std::map<vk::ShaderStageFlagBits, Source>;
    StageSources            m_Sources;
    std::vector<BindBuffer> m_Buffers;
    std::vector<BindVertex> m_Vertices;
  };

private:


};

} // namespace VPP

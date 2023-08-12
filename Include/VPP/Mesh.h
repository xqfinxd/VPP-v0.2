#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "Handle.h"

// clang-format off

namespace VPP {

class Mesh {
public:
  Mesh() {}
  ~Mesh() {}

private:
  uint32_t  m_VertexCount = 0;
  uint32_t  m_IndexCount = 0;

  std::unique_ptr<uint32_t[]>   m_Indices;
  std::unique_ptr<glm::vec3[]>  m_Vertices;
  std::unique_ptr<glm::vec3[]>  m_Normals;
  std::unique_ptr<glm::vec4[]>  m_Tangents;
  std::unique_ptr<glm::vec4[]>  m_Colors;
  std::unique_ptr<glm::vec4[]>  m_Uvs;
  std::unique_ptr<glm::vec4[]>  m_Uv2s;
  std::unique_ptr<glm::vec4[]>  m_Uv3s;
  std::unique_ptr<glm::vec4[]>  m_Uv4s;
  std::unique_ptr<glm::vec4[]>  m_Uv5s;
  std::unique_ptr<glm::vec4[]>  m_Uv6s;
  std::unique_ptr<glm::vec4[]>  m_Uv7s;
  std::unique_ptr<glm::vec4[]>  m_Uv8s;
};

struct tagMesh {};
using MeshID = Handle<tagMesh>;
using MeshManager = HandleManager<Mesh, MeshID>;

} // namespace VPP

// clang-format on

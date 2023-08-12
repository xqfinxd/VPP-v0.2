#pragma once

#include "Handle.h"
#include "Shader.h"

// clang-format on

namespace VPP {

class Material {
public:
  Material() {}
  ~Material() {}

private:
  ShaderID m_ShaderID;
};

struct tagMaterial {};
using MaterialID = Handle<tagMaterial>;
using MaterialManager = HandleManager<Material, MaterialID>;

} // namespace VPP

// clang-format on
#pragma once

#pragma once

#include "Component.h"
#include "Material.h"

// clang-format off

namespace VPP {

class MeshRenderer : public Component {
public:
  MeshRenderer() {}
  ~MeshRenderer() {}

  void SetMaterial(MaterialID material){}

private:
  MaterialID m_MaterialID;
};

} // namespace VPP

// clang-format on
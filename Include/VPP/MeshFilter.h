#pragma once

#include "Component.h"
#include "Mesh.h"

// clang-format off

namespace VPP {

class MeshFilter : public Component {
public:
  MeshFilter() {}
  ~MeshFilter() {}

  void SetMesh(MeshID mesh){}

private:
  MeshID m_MeshID;
};

} // namespace VPP

// clang-format on
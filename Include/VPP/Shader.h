#pragma once

#include "Handle.h"

// clang-format on

namespace VPP {

class Shader {
public:
  Shader() {}
  ~Shader() {}

private:
  
};

struct tagShader {};
using ShaderID = Handle<tagShader>;
using ShaderManager = HandleManager<Shader, ShaderID>;

} // namespace VPP

// clang-format on
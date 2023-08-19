#include "Shader.h"

#include <map>
#include <fstream>
#include <functional>
#include <lua.hpp>

#include "ShaderImpl.h"
#include <iostream>

namespace VPP {

void ShaderImplDeleter::operator()(ShaderImpl* impl) const {
  delete impl;
}

} // namespace VPP

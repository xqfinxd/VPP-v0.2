#pragma once

#include "Handle.h"
#include <string>

// clang-format off

namespace VPP {

class ShaderImpl;
class Shader {
public:
  Shader() {}
  ~Shader() {}

  Shader(const Shader&) = delete;
  Shader(Shader&&) noexcept = default;

  const char* GetName() const {
    return m_Name.c_str();
  }

private:
  std::string m_Name;
  ShaderImpl* m_Impl = nullptr;
};

struct tagShader {};
using ShaderID = Handle<tagShader>;
class ShaderManager {
public:
  void LoadFile(const char* file);
  ShaderID FindShader(const std::string& name);

private:
  struct NameCompare {
    bool operator()(const std::string& l, const std::string& r) const {
      return _stricmp(l.c_str(), r.c_str()) < 0;
    }
  };
  using HMgr = HandleManager<Shader, ShaderID>;
  using NameMap = std::map<std::string, ShaderID, NameCompare>;
  HMgr    m_Shaders;
  NameMap m_NameMap;
};

} // namespace VPP

// clang-format on
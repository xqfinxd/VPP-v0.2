#pragma once

#include <vector>

#ifdef SHADER_EXPORTS
#define SHADER_API __declspec(dllexport)
#else
#define SHADER_API __declspec(dllimport)
#endif  // SHADER_EXPORT

namespace VPP {

namespace impl {

class ShaderData;
class ShaderReader;

SHADER_API ShaderReader* LoadShader(std::vector<const char*> files);
SHADER_API void          DestroyShader(ShaderReader* loader);
SHADER_API ShaderData*   GetShaderData(ShaderReader* loader);

}  // namespace impl

}  // namespace VPP

#pragma once

#include <vector>

#ifdef SHADER_EXPORTS
#define SHADER_API __declspec(dllexport)
#else
#define SHADER_API __declspec(dllimport)
#endif  // SHADER_EXPORTS

class ShaderData;
class ShaderLoader;

SHADER_API ShaderLoader* CreateShaderLoader(std::vector<const char*> files);

SHADER_API void DestroyShaderLoader(ShaderLoader* loader);

SHADER_API void GetShaderData(const ShaderLoader* loader, ShaderData* data);
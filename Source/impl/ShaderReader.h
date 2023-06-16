#pragma once

#include <vector>

#ifdef SHADERREADER_EXPORTS
#define SHADER_API __declspec(dllexport)
#else
#define SHADER_API __declspec(dllimport)
#endif  // SHADER_EXPORTS

class ShaderData;
class ShaderReader;

SHADER_API ShaderReader* ShaderReader_Create(std::vector<const char*> files);

SHADER_API void ShaderReader_Destroy(ShaderReader* loader);

SHADER_API void ShaderReader_GetData(const ShaderReader* loader, ShaderData* data);
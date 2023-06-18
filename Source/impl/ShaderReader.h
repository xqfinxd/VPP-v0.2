#pragma once

#include <vector>

#ifdef SHADERREADER_EXPORTS
#define SHADER_API __declspec(dllexport)
#else
#define SHADER_API __declspec(dllimport)
#endif // SHADER_EXPORTS

namespace Shader {

struct MetaData;
struct ReaderImpl;

class SHADER_API Reader {
public:
  Reader();
  Reader(std::vector<const char*> files);
  ~Reader();

  bool GetData(MetaData* data);

private:
  ReaderImpl* impl_ = nullptr;
};
} // namespace Shader

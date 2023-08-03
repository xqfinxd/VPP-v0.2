#pragma once

#include <vector>
#include <string>
#include <map>

#ifdef VPPSHADER_EXPORTS
#define SHADER_API __declspec(dllexport)
#else
#define SHADER_API __declspec(dllimport)
#endif // SHADER_EXPORTS

#include <vulkan/vulkan.hpp>

enum VertexType {
  eVerteices,
  eNormals,
  eTangents,
  eColors,
  eUvs,
  eUvs2,
  eUvs3,
  eUvs4,
  eUvs5,
  eUvs6,
  eUvs7,
  eUvs8,
  VertexMaxCount,
};

class SHADER_API ShaderInterpreter {
public:
  typedef std::vector<vk::DescriptorSetLayoutBinding> Bindings;
  typedef std::vector<vk::ShaderStageFlagBits> Stages;
  typedef std::vector<uint32_t> Spirv;

  ShaderInterpreter(std::vector<const char*> files) {}
  virtual ~ShaderInterpreter() {}

  virtual uint32_t GetDescriptorSetCount() const = 0;
  virtual Bindings GetDescriptorSetBindings(uint32_t index) const = 0;
  virtual Stages GetAllStages() const = 0;
  virtual Spirv GetStageSpirv(vk::ShaderStageFlagBits) const = 0;

protected:
  virtual bool Success() const = 0;
};

SHADER_API std::unique_ptr<ShaderInterpreter>
ParseGlslShaders(const std::vector<const char*>&);

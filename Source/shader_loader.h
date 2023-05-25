#pragma once

#include <map>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "public/singleton.h"

namespace VPP {

namespace impl {

struct ShaderObject {
  vk::PipelineLayout                                  pipeLayout{};
  uint32_t                                            setCount = 0;
  std::unique_ptr<vk::DescriptorSetLayout[]>          setLayouts{};
  std::unique_ptr<vk::DescriptorSet[]>                setObjects{};
  std::unique_ptr<uint32_t[]>                         setNums{};
  vk::DescriptorPool                                  pool{};
  std::map<vk::ShaderStageFlagBits, vk::ShaderModule> shaderModules{};

  void setSetCount(uint32_t count);
  void destroy(const vk::Device& device);

  static std::unique_ptr<ShaderObject> createFromFiles(
      const vk::Device& device, std::map<vk::ShaderStageFlagBits, const char*> files);
};

}  // namespace impl

}  // namespace VPP

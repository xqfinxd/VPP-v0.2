#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.h"

class ShaderInterpreter;

namespace VPP {

namespace impl {

class VertexArray;

class PipelineInfo : public DeviceResource {
  friend class DrawParam;

public:
  PipelineInfo(Device* parent);
  ~PipelineInfo();

  std::vector<vk::PipelineShaderStageCreateInfo> shaders() const {
    return shaders_;
  }

  vk::PipelineLayout pipeline_layout() const { return pipe_layout_; }

  bool SetShader(const ShaderInterpreter* data);
  
  bool CreateDescriptorSets(vk::DescriptorPool& pool,
                            std::vector<vk::DescriptorSet>& sets) {
    auto poolCI = vk::DescriptorPoolCreateInfo()
                      .setMaxSets((uint32_t)desc_layout_.size())
                      .setPoolSizes(pool_sizes_);
    pool = device().createDescriptorPool(poolCI);
    if (!pool)
      return false;

     auto descAI =
        vk::DescriptorSetAllocateInfo().setDescriptorPool(pool).setSetLayouts(
            desc_layout_);
    sets.reserve(desc_layout_.size());
    sets.resize(desc_layout_.size());
    sets = device().allocateDescriptorSets(descAI);
    return !sets.empty();
  }

private:
  vk::Pipeline pipeline_{};
  vk::PipelineLayout pipe_layout_{};
  std::vector<vk::DescriptorSetLayout> desc_layout_{};
  std::vector<vk::DescriptorPoolSize> pool_sizes_{};

  std::vector<vk::PipelineShaderStageCreateInfo> shaders_{};
  std::vector<vk::VertexInputBindingDescription> vertex_bindings_{};
  std::vector<vk::VertexInputAttributeDescription> vertex_attribs_{};
};

} // namespace impl

} // namespace VPP

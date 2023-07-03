#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.h"
#include "ShaderData.h"

namespace VPP {

namespace impl {

class Program : public DeviceResource {
  friend class DrawParam;

public:
  Program(Device* parent);
  ~Program();

  bool SetShader(const Shader::MetaData& data);
  void SetVertexAttrib(uint32_t location, uint32_t binding, vk::Format format,
                       uint32_t offset);
  void SetVertexBinding(uint32_t binding, uint32_t stride,
                        vk::VertexInputRate inputRate);
  vk::Pipeline       CreateForRenderPass(vk::RenderPass renderpass) const;
  vk::DescriptorPool CreateDescriptorPool() const;
  std::vector<vk::DescriptorSet>
  AllocateDescriptorSets(const vk::DescriptorPool& pool) const;

private:
  struct ShaderObject {
    vk::ShaderModule        shader{};
    vk::ShaderStageFlagBits stage{};
  };
  vk::PipelineLayout                               pipeline_layout_{};
  std::vector<vk::DescriptorSetLayout>             descriptor_set_layout_{};
  std::vector<vk::DescriptorPoolSize>              descriptor_pool_sizes_{};
  std::vector<ShaderObject>                        shaders_{};
  std::vector<vk::VertexInputBindingDescription>   vertex_bindings_{};
  std::vector<vk::VertexInputAttributeDescription> vertex_attribs_{};
};

} // namespace impl

} // namespace VPP

#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.h"
#include "ShaderData.h"

namespace VPP {

namespace impl {

class Pipeline : public DeviceResource {
  friend class DrawParam;

public:
  Pipeline(Device* parent);
  ~Pipeline();

  bool SetShader(const Shader::MetaData& data);
  void SetVertexAttrib(uint32_t location, uint32_t binding, vk::Format format,
                       uint32_t offset);
  void SetVertexBinding(uint32_t binding, uint32_t stride,
                        vk::VertexInputRate inputRate);
  vk::Pipeline CreateForRenderPass(vk::RenderPass renderpass);

private:
  struct Module {
    vk::ShaderModule        shader{};
    vk::ShaderStageFlagBits stage{};
  };
  std::vector<vk::Pipeline>                        pipelines_;
  vk::PipelineLayout                               pipe_layout_{};
  std::vector<vk::DescriptorSetLayout>             desc_layout_{};
  vk::DescriptorPool                               descriptor_pool_{};
  std::vector<vk::DescriptorSet>                   descriptor_sets_{};
  std::vector<Module>                              shaders_{};
  std::vector<vk::VertexInputBindingDescription>   vertex_bindings_{};
  std::vector<vk::VertexInputAttributeDescription> vertex_attribs_{};
};

} // namespace impl

} // namespace VPP

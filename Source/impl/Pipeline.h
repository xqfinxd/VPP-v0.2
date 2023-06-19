#pragma once

#include <vulkan/vulkan.hpp>

#include "Buffer.h"
#include "Device.h"
#include "ShaderData.h"

namespace VPP {

namespace impl {

class Pipeline : public DeviceResource {
  friend class DrawCmd;

public:
  Pipeline();
  ~Pipeline();

  bool SetShader(const Shader::MetaData& data);
  void SetVertexArray(const VertexArray& array);
  void SetVertexAttrib(uint32_t location, uint32_t binding, vk::Format format,
                       uint32_t offset);
  bool Enable();

private:
  struct Module {
    vk::ShaderModule shader{};
    vk::ShaderStageFlagBits stage{};
  };
  vk::Pipeline pipeline_{};
  vk::PipelineLayout pipe_layout_{};
  std::vector<vk::DescriptorSetLayout> desc_layout_{};
  vk::DescriptorPool desc_pool_{};
  std::vector<vk::DescriptorSet> desc_set_{};
  std::vector<Module> shaders_{};
  std::vector<vk::VertexInputBindingDescription> vertex_bindings_{};
  std::vector<vk::VertexInputAttributeDescription> vertex_attribs_{};
};

class DrawCmd : public DeviceResource {
public:
  DrawCmd(uint32_t index, const std::vector<vk::ClearValue>& clearValues);
  DrawCmd(DrawCmd&) = delete;
  DrawCmd(DrawCmd&& dc) noexcept {
    buf_ = dc.buf_;
    dc.buf_ = nullptr;
  }
  ~DrawCmd();

  void BindPipeline(const Pipeline& pipeline);

  void DrawVertex(const VertexArray& vertex);

  void SetViewport();
  void SetScissor();

private:
  const vk::CommandBuffer* buf_{};
};

} // namespace impl

} // namespace VPP

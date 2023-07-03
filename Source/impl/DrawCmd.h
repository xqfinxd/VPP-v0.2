#pragma once

#include <vulkan/vulkan.hpp>

#include <map>

#include "Buffer.h"
#include "Device.h"
#include "Image.h"
#include "Pipeline.h"

namespace VPP {

namespace impl {

class DrawParam : public DeviceResource {
public:
  void SetClearValues(std::vector<vk::ClearValue>& clearValues) {
    clear_values_.swap(clearValues);
  }
  void UseProgram(const Program* program);
  bool BindTexture(uint32_t slot, uint32_t set, uint32_t binding);
  bool BindBlock(uint32_t slot, uint32_t set, uint32_t binding);
  void Call(const vk::CommandBuffer& buf, const vk::Framebuffer& framebuffer,
            const vk::RenderPass& renderpass, uint32_t subpass, vk::Rect2D area,
            vk::Viewport viewport, vk::Rect2D scissor,
            const VertexArray* vertices) const;

private:
  vk::Pipeline                   pipeline_;
  vk::DescriptorPool             descriptor_pool_;
  std::vector<vk::DescriptorSet> descriptor_sets_;
  const Program*                 program_;

  std::vector<std::pair<uint32_t, const SamplerTexture*>> sampler_textures_{};
  std::vector<std::pair<uint32_t, const UniformBuffer*>>  uniform_buffers_{};
  std::vector<vk::ClearValue>                             clear_values_{};
};

} // namespace impl

} // namespace VPP

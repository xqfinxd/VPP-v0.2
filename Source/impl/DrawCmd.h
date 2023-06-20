#pragma once

#include <vulkan/vulkan.hpp>

#include "Buffer.h"
#include "Device.h"
#include "Pipeline.h"

namespace VPP {

namespace impl {

class DrawCmd : public DeviceResource {
public:
  void set_clear_values(std::vector<vk::ClearValue>& clearValues) {
    clear_values_.swap(clearValues);
  }
  void set_vertices(const VertexArray& vertex) { vertices_ = &vertex; }
  void set_pipeline(const Pipeline& pipeline) { pipeline_ = &pipeline; }
  void set_scissors(std::vector<vk::Rect2D>& scissors) {
    scissors_.swap(scissors);
  }
  void Call(const vk::CommandBuffer& buf, const vk::Framebuffer& framebuffer,
            const vk::RenderPass& renderpass) const;

private:
  const VertexArray* vertices_ = nullptr;
  const Pipeline* pipeline_ = nullptr;
  std::vector<vk::ClearValue> clear_values_{};
  std::vector<vk::Rect2D> scissors_{};
};

} // namespace impl

} // namespace VPP

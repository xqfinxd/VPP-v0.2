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
  DrawParam(Device* parent) : DeviceResource(parent) {}

  void SetClearValues(std::vector<vk::ClearValue>& clearValues) {
    clear_values_.swap(clearValues);
  }
  void SetVertexArray(VertexArray& vertex) { vertices_ = &vertex; }
  void SetPipeline(Pipeline& pipeline) {
    if (vertices_ && pipeline.Enable(*vertices_))
      pipeline_ = &pipeline;
  }
  void SetTexture(uint32_t slot, SamplerTexture& tex) {
    auto iter = std::find_if(
        sampler_textures_.begin(), sampler_textures_.end(),
        [slot](const std::pair<uint32_t, const SamplerTexture*>& e) {
          return e.first == slot;
        });
    if (iter == sampler_textures_.end()) {
      sampler_textures_.emplace_back(std::make_pair(slot, &tex));
    } else {
      iter->second = &tex;
    }
  }
  void SetUniform(uint32_t slot, UniformBuffer& buf) {
      auto iter = std::find_if(
          uniform_buffers_.begin(), uniform_buffers_.end(),
          [slot](const std::pair<uint32_t, const UniformBuffer*>& e) {
          return e.first == slot;
      });
      if (iter == uniform_buffers_.end()) {
          uniform_buffers_.emplace_back(std::make_pair(slot, &buf));
      } else {
          iter->second = &buf;
      }
  }

  bool BindTexture(uint32_t slot, uint32_t set, uint32_t binding);
  bool BindUniform(uint32_t slot, uint32_t set, uint32_t binding); // ¾¯¸æ£ºdescriptorCount±»ºöÂÔ

  void Call(const vk::CommandBuffer& buf, const vk::Framebuffer& framebuffer,
            const vk::RenderPass& renderpass) const;

private:
  const VertexArray* vertices_ = nullptr;
  const Pipeline* pipeline_ = nullptr;
  std::vector<std::pair<uint32_t, const SamplerTexture*>> sampler_textures_{};
  std::vector<std::pair<uint32_t, const UniformBuffer*>> uniform_buffers_{};
  std::vector<vk::ClearValue> clear_values_{};
};

} // namespace impl

} // namespace VPP

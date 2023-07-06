#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.h"

namespace VPP {
namespace impl {
class SamplerTexture : public DeviceResource {
public:
  SamplerTexture(Device* parent);
  ~SamplerTexture();

  bool SetImage2D(vk::Format format, uint32_t width, uint32_t height,
                  uint32_t channel, const void* data);

  const vk::ImageView& view() const { return view_; }
  const vk::Sampler& sampler() const { return sampler_; }

private:
  vk::Format format_ = vk::Format::eUndefined;
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  vk::Image image_{};
  vk::ImageView view_{};
  vk::DeviceMemory memory_{};
  vk::Sampler sampler_{};
};
} // namespace impl
} // namespace VPP
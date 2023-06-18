#include <vulkan/vulkan.hpp>

#include "Device.h"

namespace VPP {
namespace impl {
class CombinedImageSampler : public DeviceResource {
public:
  CombinedImageSampler();
  ~CombinedImageSampler();

  bool Init(vk::Format format, uint32_t width, uint32_t height, void* data,
            size_t size);

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
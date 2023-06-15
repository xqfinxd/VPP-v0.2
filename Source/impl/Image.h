#include <vulkan/vulkan.hpp>

#include "Device.h"

namespace VPP {
namespace impl {
class Image : public DeviceResource {
 public:
  Image();
  ~Image();

  bool Init(vk::Format format, uint32_t width, uint32_t height);
  bool SetData(void* data, size_t size);

 private:
  vk::Format format_ = vk::Format::eUndefined;
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  vk::DeviceSize memory_size_ = 0;
  vk::Image image_{};
  vk::ImageView view_{};
  vk::DeviceMemory memory_{};
};

class Sampler : public DeviceResource {
 public:
  Sampler();
  ~Sampler();

  bool Init();

 private:
  vk::Sampler sampler_{};
};
}  // namespace impl
}  // namespace VPP
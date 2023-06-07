#include <vulkan/vulkan.hpp>

#include "renderer_impl.h"

namespace VPP {

namespace impl {

class Buffer {
 public:
  Buffer();
  ~Buffer();

  bool Init(vk::BufferUsageFlags usage, vk::DeviceSize size);
  bool SetData(void* data, size_t size);

 private:
  vk::BufferUsageFlags usage_ = (vk::BufferUsageFlags)0;
  vk::DeviceSize       size_ = 0;
  vk::Buffer           buffer_{};
  vk::DeviceMemory     memory_{};
};

}  // namespace impl

}  // namespace VPP
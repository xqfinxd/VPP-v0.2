#include <vulkan/vulkan.hpp>

#include "Device.h"

namespace VPP {
namespace impl {
class Buffer : public DeviceResource {
 public:
  Buffer();
  ~Buffer();

  bool Init(vk::BufferUsageFlags usage, vk::DeviceSize size);
  bool Init(vk::BufferUsageFlags usage, vk::DeviceSize size,
            vk::MemoryPropertyFlags memoryFlags);
  bool SetData(void* data, size_t size);

 private:
  vk::BufferUsageFlags    usage_ = (vk::BufferUsageFlags)0;
  vk::DeviceSize          size_ = 0;
  vk::MemoryPropertyFlags memory_flags_;

  vk::Buffer              buffer_{};
  vk::DeviceMemory        memory_{};
};
}  // namespace impl
}  // namespace VPP
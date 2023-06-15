#include <vulkan/vulkan.hpp>

#include "Device.h"

namespace VPP {
namespace impl {
class VertexBuffer : public DeviceResource {
 public:
  VertexBuffer();
  ~VertexBuffer();

  bool Init(uint32_t stride, uint32_t count, void* data, size_t size);

 private:
  uint32_t vertex_stride_ = 0;
  uint32_t vertex_count_ = 0;

  vk::Buffer buffer_{};
  vk::DeviceMemory memory_{};
};

class IndexBuffer : public DeviceResource {
 public:
  IndexBuffer();
  ~IndexBuffer();

  bool Init(uint32_t count, void* data, size_t size);

 private:
  uint32_t index_count_ = 0;

  vk::Buffer buffer_{};
  vk::DeviceMemory memory_{};
};

class UniformBuffer : public DeviceResource {
 public:
  UniformBuffer();
  ~UniformBuffer();

  bool Init(size_t size);

 private:
  size_t size_ = 0;
  std::vector<uint8_t> data_{};

  vk::Buffer buffer_{};
  vk::DeviceMemory memory_{};
};
}  // namespace impl
}  // namespace VPP
#include <vulkan/vulkan.hpp>

#include "Device.h"

namespace VPP {
namespace impl {
class VertexBuffer : public DeviceResource {
  friend class VertexArray;

public:
  VertexBuffer();
  ~VertexBuffer();

  bool SetData(uint32_t stride, uint32_t count, void* data, size_t size);

private:
  uint32_t vertex_stride_ = 0;
  uint32_t vertex_count_ = 0;

  vk::Buffer buffer_{};
  vk::DeviceMemory memory_{};
};

class IndexBuffer : public DeviceResource {
  friend class VertexArray;

public:
  IndexBuffer();
  ~IndexBuffer();

  bool SetData(uint32_t count, void* data, size_t size);

private:
  uint32_t index_count_ = 0;

  vk::Buffer buffer_{};
  vk::DeviceMemory memory_{};
};

class VertexArray {
  friend class DrawCmd;

public:
  void BindVertex(const VertexBuffer& vertex);
  void BindIndex(const IndexBuffer& index);
  std::vector<vk::VertexInputBindingDescription> GetBindings() const;
  std::vector<vk::Buffer> GetVertex() const;
  const vk::Buffer& GetIndex() const;
  uint32_t GetIndexCount() const {
    return index_->index_count_;
  }

private:
  std::vector<const VertexBuffer*> vertices_{};
  const IndexBuffer* index_ = nullptr;
};

class UniformBuffer : public DeviceResource {
public:
  UniformBuffer();
  ~UniformBuffer();

  bool SetData(size_t size);

private:
  size_t size_ = 0;
  std::vector<uint8_t> data_{};

  vk::Buffer buffer_{};
  vk::DeviceMemory memory_{};
};
} // namespace impl
} // namespace VPP
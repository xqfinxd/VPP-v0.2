#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.h"

namespace VPP {
namespace impl {
class VertexBuffer : public DeviceResource {
public:
  ~VertexBuffer();

  bool SetData(uint32_t stride, uint32_t count, void* data, size_t size);

  uint32_t stride() const {
    return stride_;
  }

  uint32_t count() const {
    return count_;
  }

  const vk::Buffer& buffer() const {
    return buffer_;
  }

private:
  uint32_t stride_ = 0;
  uint32_t count_ = 0;

  vk::Buffer buffer_{};
  vk::DeviceMemory memory_{};
};

class IndexBuffer : public DeviceResource {
public:
  ~IndexBuffer();

  bool SetData(uint32_t count, void* data, size_t size);

  uint32_t count() const {
    return count_;
  }

  const vk::Buffer& buffer() const {
    return buffer_;
  }

private:
  uint32_t count_ = 0;

  vk::Buffer buffer_{};
  vk::DeviceMemory memory_{};
};

class VertexArray : public DeviceResource {
public:
  void add_vertex(const VertexBuffer& vertex);
  void set_index(const IndexBuffer& index);
  void BindCmd(const vk::CommandBuffer& buf) const;
  void DrawAtCmd(const vk::CommandBuffer& buf) const;

  std::vector<vk::VertexInputBindingDescription> GetBindings() const;

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
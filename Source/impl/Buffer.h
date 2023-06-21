#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.h"

namespace VPP {
namespace impl {

class StaticBuffer : public DeviceResource {
public:
  const vk::Buffer& buffer() const { return buffer_; }

protected:
  StaticBuffer();
  ~StaticBuffer();
  bool SetData(vk::BufferUsageFlags usage, const void* data, size_t size);

private:
  vk::Buffer buffer_{};
  vk::DeviceMemory memory_{};
};

class VertexBuffer : public StaticBuffer {
public:
  bool SetData(uint32_t stride, uint32_t count, const void* data, size_t size);

  uint32_t stride() const { return stride_; }
  uint32_t count() const { return count_; }

private:
  uint32_t stride_ = 0;
  uint32_t count_ = 0;
};

class IndexBuffer : public StaticBuffer {
public:
  bool SetData(uint32_t count, const void* data, size_t size);

  uint32_t count() const { return count_; }

private:
  uint32_t count_ = 0;
};

class VertexArray : public DeviceResource {
public:
  void BindBuffer(const VertexBuffer& vertex);
  void BindBuffer(const IndexBuffer& index);
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
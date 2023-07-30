#pragma once

#include <vulkan/vulkan.hpp>

#include "Device.h"

namespace VPP {
namespace impl {

class CommonBuffer : public DeviceResource {
public:
  const vk::Buffer& buffer() const { return buffer_; }
  const vk::DeviceMemory& memory() const { return memory_; }

protected:
  CommonBuffer(Device* parent);
  ~CommonBuffer();
  bool SetLocalData(vk::BufferUsageFlags usage, const void* data, size_t size);
  bool SetGlobalData(vk::BufferUsageFlags usage, const void* data, size_t size);

private:
  vk::Buffer buffer_{};
  vk::DeviceMemory memory_{};
};

class VertexBuffer : public CommonBuffer {
public:
  VertexBuffer(Device* parent) : CommonBuffer(parent) {}
  bool SetData(uint32_t stride, uint32_t count, const void* data, size_t size);

  uint32_t stride() const { return stride_; }
  uint32_t count() const { return count_; }

private:
  uint32_t stride_ = 0;
  uint32_t count_ = 0;
};

class IndexBuffer : public CommonBuffer {
public:
  IndexBuffer(Device* parent) : CommonBuffer(parent) {}
  bool SetData(uint32_t count, const void* data, size_t size);

  uint32_t count() const { return count_; }

private:
  uint32_t count_ = 0;
};

class VertexArray : public DeviceResource {
public:
  VertexArray(Device* parent) : DeviceResource(parent) {}

  void BindBuffer(const VertexBuffer& vertex);
  void BindBuffer(const IndexBuffer& index);
  void BindCmd(const vk::CommandBuffer& buf) const;
  void DrawAtCmd(const vk::CommandBuffer& buf) const;

  std::vector<vk::VertexInputBindingDescription> GetBindings() const {
    std::vector<vk::VertexInputBindingDescription> bindings{};
    uint32_t index = 0;
    for (const auto& e : vertices_) {
      bindings.emplace_back(vk::VertexInputBindingDescription()
                                .setBinding(index++)
                                .setStride(e->stride())
                                .setInputRate(vk::VertexInputRate::eVertex));
    }
    return bindings;
  }
  std::vector<vk::VertexInputAttributeDescription> GetAttrib() const {
    return vertex_attribs_;
  }
  void SetVertexAttrib(uint32_t location, uint32_t binding, vk::Format format,
    uint32_t offset) {
    vertex_attribs_.emplace_back(vk::VertexInputAttributeDescription()
                                     .setLocation(location)
                                     .setBinding(binding)
                                     .setFormat(format)
                                     .setOffset(offset));
  }

private:
  std::vector<const VertexBuffer*> vertices_{};
  const IndexBuffer* index_ = nullptr;
  std::vector<vk::VertexInputAttributeDescription> vertex_attribs_{};
};

class UniformBuffer : public CommonBuffer {
public:
  UniformBuffer(Device* parent) : CommonBuffer(parent) {}

  bool SetData(size_t size);
  size_t size() const { return size_; }
  void UpdateData(void* data, size_t size);

private:
  size_t size_ = 0;
  std::vector<uint8_t> data_{};
};
} // namespace impl
} // namespace VPP
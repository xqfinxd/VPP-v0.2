#include "Buffer.h"

namespace VPP {
namespace impl {

CommonBuffer::CommonBuffer(Device* parent) : DeviceResource(parent) {}

CommonBuffer::~CommonBuffer() {
  if (buffer_) {
    device().destroy(buffer_);
  }
  if (memory_) {
    device().free(memory_);
  }
}

bool CommonBuffer::SetStaticData(vk::BufferUsageFlags usage, const void* data,
                                 size_t size) {
  buffer_ = CreateBuffer(usage | vk::BufferUsageFlagBits::eTransferDst, size);
  if (!buffer_) {
    return false;
  }

  memory_ = CreateMemory(device().getBufferMemoryRequirements(buffer_),
                         vk::MemoryPropertyFlagBits::eDeviceLocal);
  if (!memory_) {
    return false;
  }

  device().bindBufferMemory(buffer_, memory_, 0);

  auto stageBuffer = CreateStageBuffer(data, size);
  if (!stageBuffer) {
    return false;
  }
  return stageBuffer->CopyToBuffer(buffer_, size);
}

bool CommonBuffer::SetDynamicData(vk::BufferUsageFlags usage, const void* data,
                                  size_t size) {
  auto result = vk::Result::eSuccess;

  buffer_ = CreateBuffer(vk::BufferUsageFlagBits::eUniformBuffer, size);
  if (!buffer_) {
    return false;
  }

  auto req = device().getBufferMemoryRequirements(buffer_);

  vk::MemoryPropertyFlags memFlags = vk::MemoryPropertyFlagBits::eHostVisible |
                                     vk::MemoryPropertyFlagBits::eHostCoherent;
  memory_ = CreateMemory(req, memFlags);
  if (!memory_) {
    return false;
  }

  device().bindBufferMemory(buffer_, memory_, 0);

  if (data) {
    void* mapData = device().mapMemory(memory_, 0, size, vk::MemoryMapFlags());
    if (mapData) {
      memcpy(mapData, data, size);
    }
    device().unmapMemory(memory_);
  }

  return true;
}

VertexBuffer::VertexBuffer(Device* parent) : CommonBuffer(parent) {}

bool VertexBuffer::SetData(uint32_t stride, uint32_t count, const void* data,
                           size_t size) {
  stride_ = stride;
  count_  = count;

  return SetStaticData(vk::BufferUsageFlagBits::eVertexBuffer, data, size);
}

IndexBuffer::IndexBuffer(Device* parent) : CommonBuffer(parent) {}

bool IndexBuffer::SetData(uint32_t count, const void* data, size_t size) {
  count_ = count;

  return SetStaticData(vk::BufferUsageFlagBits::eIndexBuffer, data, size);
}

UniformBuffer::UniformBuffer(Device* parent) : CommonBuffer(parent) {}

bool UniformBuffer::SetData(size_t size) {
  size_ = size;

  auto result = vk::Result::eSuccess;
  return SetDynamicData(vk::BufferUsageFlagBits::eUniformBuffer, nullptr, size);
}

void UniformBuffer::UpdateData(void* data, size_t size) {
  if (!data || !size) {
    return;
  }
  auto   mapData  = device().mapMemory(memory(), 0, size_);
  size_t safeSize = std::min(size_, size);
  memcpy(mapData, data, safeSize);
  device().unmapMemory(memory());
}

void VertexArray::BindBuffer(const VertexBuffer& vertex) {
  vertices_.push_back(&vertex);
}

void VertexArray::BindBuffer(const IndexBuffer& index) { index_ = &index; }

bool VertexArray::Compatible(
    const std::vector<vk::VertexInputBindingDescription>& bindings) const {
  for (const auto& e : bindings) {
    if (e.binding >= vertices_.size()) {
      return false;
    }
    if (vertices_[e.binding]->stride() != e.stride) {
      return false;
    }
  }
  return true;
}

} // namespace impl
} // namespace VPP
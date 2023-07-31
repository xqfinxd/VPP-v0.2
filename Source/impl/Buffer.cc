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

bool CommonBuffer::SetLocalData(vk::BufferUsageFlags usage, const void* data,
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

  StageBuffer stageBuffer(this, data, size);
  return stageBuffer.CopyToBuffer(buffer_);
}

bool CommonBuffer::SetGlobalData(vk::BufferUsageFlags usage, const void* data, size_t size) {
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

bool VertexBuffer::SetData(uint32_t stride, uint32_t count, const void* data,
                           size_t size) {
  stride_ = stride;
  count_ = count;

  return SetLocalData(vk::BufferUsageFlagBits::eVertexBuffer, data,
                               size);
}

bool IndexBuffer::SetData(uint32_t count, const void* data, size_t size) {
  count_ = count;

  return SetLocalData(vk::BufferUsageFlagBits::eIndexBuffer, data,
                               size);
}

bool UniformBuffer::SetData(size_t size) {
  size_ = size;

  auto result = vk::Result::eSuccess;
  return SetGlobalData(vk::BufferUsageFlagBits::eUniformBuffer, nullptr, size);
}

void UniformBuffer::UpdateData(void* data, size_t size) {
    if (!data || !size) { return; }
    auto mapData = device().mapMemory(memory(), 0, size_);
    size_t safeSize = std::min(size_, size);
    memcpy(mapData, data, safeSize);
    device().unmapMemory(memory());
}

void VertexArray::BindBuffer(const VertexBuffer& vertex) {
  vertices_.push_back(&vertex);
}

void VertexArray::BindBuffer(const IndexBuffer& index) { index_ = &index; }

void VertexArray::BindCmd(const vk::CommandBuffer& buf) const {
  std::vector<vk::Buffer> buffers{};
  for (const auto& e : vertices_) {
    buffers.emplace_back(e->buffer());
  }

  std::vector<vk::DeviceSize> offsets{};
  offsets.assign(buffers.size(), 0);

  buf.bindVertexBuffers(0, buffers, offsets);
  if (index_) {
    buf.bindIndexBuffer(index_->buffer(), 0, vk::IndexType::eUint32);
  }
}

void VertexArray::DrawAtCmd(const vk::CommandBuffer& buf) const {
  if (index_) {
    buf.drawIndexed(index_->count(), 2, 0, 0, 0);
  } else {
    uint32_t minVertexCount = UINT32_MAX;
    for (const auto* e : vertices_) {
      minVertexCount = std::min(minVertexCount, e->count());
    }
    buf.draw(minVertexCount, 2, 0, 0);
  }
}

} // namespace impl
} // namespace VPP
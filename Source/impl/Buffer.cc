#include "Buffer.h"

namespace VPP {
namespace impl {
VertexBuffer::VertexBuffer() : DeviceResource() {
}

VertexBuffer::~VertexBuffer() {
  if (buffer_) {
    device().destroy(buffer_);
  }
  if (memory_) {
    device().free(memory_);
  }
}

bool VertexBuffer::Init(uint32_t stride, uint32_t count, void* data, size_t size) {
  vertex_stride_ = stride;
  vertex_count_ = count;

  auto result = vk::Result::eSuccess;

  auto bufferCI = vk::BufferCreateInfo()
                      .setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
                      .setQueueFamilyIndexCount(0)
                      .setPQueueFamilyIndices(nullptr)
                      .setSharingMode(vk::SharingMode::eExclusive)
                      .setSize(size);
  result = device().createBuffer(&bufferCI, nullptr, &buffer_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  auto req = device().getBufferMemoryRequirements(buffer_);

  vk::MemoryPropertyFlags memFlags =
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
  memory_ = CreateMemory(req, memFlags);
  if (!memory_) {
    return false;
  }

  size_t memSize = std::min(req.size, size);
  void* mapData = device().mapMemory(memory_, 0, memSize, vk::MemoryMapFlags());
  if (!mapData) {
    return false;
  }

  memcpy(mapData, data, memSize);
  device().unmapMemory(memory_);

  device().bindBufferMemory(buffer_, memory_, 0);
  return true;
}

IndexBuffer::IndexBuffer() : DeviceResource() {
}

IndexBuffer::~IndexBuffer() {
  if (buffer_) {
    device().destroy(buffer_);
  }
  if (memory_) {
    device().free(memory_);
  }
}

bool IndexBuffer::Init(uint32_t count, void* data, size_t size) {
  index_count_ = count;

  auto result = vk::Result::eSuccess;

  auto bufferCI = vk::BufferCreateInfo()
                      .setUsage(vk::BufferUsageFlagBits::eIndexBuffer)
                      .setQueueFamilyIndexCount(0)
                      .setPQueueFamilyIndices(nullptr)
                      .setSharingMode(vk::SharingMode::eExclusive)
                      .setSize(size);
  result = device().createBuffer(&bufferCI, nullptr, &buffer_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  auto req = device().getBufferMemoryRequirements(buffer_);

  vk::MemoryPropertyFlags memFlags =
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
  memory_ = CreateMemory(req, memFlags);
  if (!memory_) {
    return false;
  }

  size_t memSize = std::min(req.size, size);
  void* mapData = device().mapMemory(memory_, 0, memSize, vk::MemoryMapFlags());
  if (!mapData) {
    return false;
  }

  memcpy(mapData, data, memSize);
  device().unmapMemory(memory_);

  device().bindBufferMemory(buffer_, memory_, 0);
  return true;
}

UniformBuffer::UniformBuffer() : DeviceResource() {
}

UniformBuffer::~UniformBuffer() {
  if (buffer_) {
    device().destroy(buffer_);
  }
  if (memory_) {
    device().free(memory_);
  }
}

bool UniformBuffer::Init(size_t size) {
  size_ = size;

  auto result = vk::Result::eSuccess;

  auto bufferCI = vk::BufferCreateInfo()
                      .setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
                      .setQueueFamilyIndexCount(0)
                      .setPQueueFamilyIndices(nullptr)
                      .setSharingMode(vk::SharingMode::eExclusive)
                      .setSize(size);
  result = device().createBuffer(&bufferCI, nullptr, &buffer_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  auto req = device().getBufferMemoryRequirements(buffer_);

  vk::MemoryPropertyFlags memFlags =
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
  memory_ = CreateMemory(req, memFlags);
  if (!memory_) {
    return false;
  }

  size_t memSize = std::min(req.size, size);
  void* mapData = device().mapMemory(memory_, 0, memSize, vk::MemoryMapFlags());
  if (mapData) {
    memset(mapData, 0, memSize);
  }
  device().unmapMemory(memory_);

  device().bindBufferMemory(buffer_, memory_, 0);
  return true;
}
}  // namespace impl
}  // namespace VPP
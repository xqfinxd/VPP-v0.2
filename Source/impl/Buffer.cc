#include "Buffer.h"

namespace VPP {
namespace impl {

VertexBuffer::~VertexBuffer() {
  if (buffer_) {
    device().destroy(buffer_);
  }
  if (memory_) {
    device().free(memory_);
  }
}

bool VertexBuffer::SetData(uint32_t stride, uint32_t count, void* data,
                           size_t size) {
  stride_ = stride;
  count_ = count;

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

  vk::MemoryPropertyFlags memFlags = vk::MemoryPropertyFlagBits::eHostVisible |
                                     vk::MemoryPropertyFlagBits::eHostCoherent;
  memory_ = CreateMemory(req, memFlags);
  if (!memory_) {
    return false;
  }

  device().bindBufferMemory(buffer_, memory_, 0);

  size_t memSize = std::min(req.size, size);
  void* mapData = device().mapMemory(memory_, 0, memSize, vk::MemoryMapFlags());
  if (!mapData) {
    return false;
  }

  memcpy(mapData, data, memSize);
  device().unmapMemory(memory_);

  return true;
}

IndexBuffer::~IndexBuffer() {
  if (buffer_) {
    device().destroy(buffer_);
  }
  if (memory_) {
    device().free(memory_);
  }
}

bool IndexBuffer::SetData(uint32_t count, void* data, size_t size) {
  count_ = count;

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

  vk::MemoryPropertyFlags memFlags = vk::MemoryPropertyFlagBits::eHostVisible |
                                     vk::MemoryPropertyFlagBits::eHostCoherent;
  memory_ = CreateMemory(req, memFlags);
  if (!memory_) {
    return false;
  }

  device().bindBufferMemory(buffer_, memory_, 0);

  size_t memSize = std::min(req.size, size);
  void* mapData = device().mapMemory(memory_, 0, memSize, vk::MemoryMapFlags());
  if (!mapData) {
    return false;
  }

  memcpy(mapData, data, memSize);
  device().unmapMemory(memory_);

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

bool UniformBuffer::SetData(size_t size) {
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

  vk::MemoryPropertyFlags memFlags = vk::MemoryPropertyFlagBits::eHostVisible |
                                     vk::MemoryPropertyFlagBits::eHostCoherent;
  memory_ = CreateMemory(req, memFlags);
  if (!memory_) {
    return false;
  }

  device().bindBufferMemory(buffer_, memory_, 0);

  size_t memSize = std::min(req.size, size);
  void* mapData = device().mapMemory(memory_, 0, memSize, vk::MemoryMapFlags());
  if (mapData) {
    memset(mapData, 0, memSize);
  }
  device().unmapMemory(memory_);

  return true;
}

void VertexArray::add_vertex(const VertexBuffer& vertex) {
  vertices_.push_back(&vertex);
}

void VertexArray::set_index(const IndexBuffer& index) {
  index_ = &index;
}

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
    buf.drawIndexed(index_->count(), 1, 0, 0, 0);
  } else {
    uint32_t minVertexCount = UINT32_MAX;
    for (const auto* e : vertices_) {
      minVertexCount = std::min(minVertexCount, e->count());
    }
    buf.draw(minVertexCount, 1, 0, 0);
  }
}

std::vector<vk::VertexInputBindingDescription>
VertexArray::GetBindings() const {
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

} // namespace impl
} // namespace VPP
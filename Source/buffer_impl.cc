#include "buffer_impl.h"

#include "renderer_impl.h"

namespace VPP {

namespace impl {

vk::MemoryPropertyFlags GetMemoryTypeByUsage(vk::BufferUsageFlags usage) {
    if (usage & vk::BufferUsageFlagBits::eVertexBuffer) {
        return vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent;
    }
    return (vk::MemoryPropertyFlags)0;
}

UniformBuffer::UniformBuffer() {}

UniformBuffer::~UniformBuffer() {}

bool UniformBuffer::Init(vk::BufferUsageFlags usage, vk::DeviceSize size) {
  usage_ = usage;
  size_ = size;
  auto& device = Renderer::GetMe().device();

  vk::Result result = vk::Result::eSuccess;

  auto bufferCI = vk::BufferCreateInfo()
                      .setUsage(usage)
                      .setQueueFamilyIndexCount(0)
                      .setPQueueFamilyIndices(nullptr)
                      .setSharingMode(vk::SharingMode::eExclusive)
                      .setSize(size);
  result = device.createBuffer(&bufferCI, nullptr, &buffer_);
  if (result != vk::Result::eSuccess) {
    return false;
  }
  vk::MemoryRequirements req;
  req = device.getBufferMemoryRequirements(buffer_);
  auto allocateInfo = vk::MemoryAllocateInfo().setAllocationSize(req.size);

  vk::MemoryPropertyFlags memFlags = GetMemoryTypeByUsage(usage_);
  auto found = Renderer::GetMe().FindMemoryType(req.memoryTypeBits, memFlags,
                                                allocateInfo.memoryTypeIndex);
  if (!found) {
    return false;
  }
  result = device.allocateMemory(&allocateInfo, nullptr, &memory_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  device.bindBufferMemory(buffer_, memory_, 0);
  return true;
}

bool UniformBuffer::SetData(void* data, size_t size) {
  auto&      device = Renderer::GetMe().device();

  vk::Result result = vk::Result::eSuccess;

  void*      mapData = nullptr;
  size_t limitSize = std::min(size, size_);
  result = device.mapMemory(memory_, 0, limitSize, vk::MemoryMapFlags(), &mapData);
  if (result != vk::Result::eSuccess) {
    return false;
  }
  memcpy(mapData, data, limitSize);
  device.unmapMemory(memory_);
}

}  // namespace impl

}  // namespace VPP
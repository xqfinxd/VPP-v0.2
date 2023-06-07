#include "buffer_impl.h"

namespace VPP {

namespace impl {

vk::MemoryPropertyFlags GetMemoryTypeByUsage(vk::BufferUsageFlags usage) {
  if (usage & vk::BufferUsageFlagBits::eVertexBuffer) {
    return vk::MemoryPropertyFlagBits::eHostVisible |
           vk::MemoryPropertyFlagBits::eHostCoherent;
  }
  return (vk::MemoryPropertyFlags)0;
}

Buffer::Buffer() {}

Buffer::~Buffer() {}

bool Buffer::Init(vk::BufferUsageFlags usage, vk::DeviceSize size) {
  usage_ = usage;
  size_ = size;

  vk::Result result = vk::Result::eSuccess;

  auto bufferCI = vk::BufferCreateInfo()
                      .setUsage(usage)
                      .setQueueFamilyIndexCount(0)
                      .setPQueueFamilyIndices(nullptr)
                      .setSharingMode(vk::SharingMode::eExclusive)
                      .setSize(size);
  result = Renderer::GetMe().device.createBuffer(&bufferCI, nullptr, &buffer_);
  if (result != vk::Result::eSuccess) {
    return false;
  }
  vk::MemoryRequirements req;
  req = Renderer::GetMe().device.getBufferMemoryRequirements(buffer_);
  auto allocateInfo = vk::MemoryAllocateInfo().setAllocationSize(req.size);

  vk::MemoryPropertyFlags memFlags = GetMemoryTypeByUsage(usage_);
  auto found = Renderer::GetMe().FindMemoryType(req.memoryTypeBits, memFlags,
                                                allocateInfo.memoryTypeIndex);
  if (!found) {
    return false;
  }
  result = Renderer::GetMe().device.allocateMemory(&allocateInfo, nullptr, &memory_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  Renderer::GetMe().device.bindBufferMemory(buffer_, memory_, 0);
  return true;
}

bool Buffer::SetData(void* data, size_t size) {
  vk::Result result = vk::Result::eSuccess;

  void*  mapData = nullptr;
  size_t limitSize = std::min(size, size_);
  result =
      Renderer::GetMe().device.mapMemory(memory_, 0, limitSize, vk::MemoryMapFlags(), &mapData);
  if (result != vk::Result::eSuccess) {
    return false;
  }
  memcpy(mapData, data, limitSize);
  Renderer::GetMe().device.unmapMemory(memory_);
  return true;
}

}  // namespace impl

}  // namespace VPP
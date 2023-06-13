#include "Buffer.h"

namespace VPP {
namespace impl {

Buffer::Buffer() {}

Buffer::~Buffer() {
  auto& device = renderer()->device();
  device.destroy(buffer_);
  device.free(memory_);
}

bool Buffer::Init(vk::BufferUsageFlags usage, vk::DeviceSize size) {
  return Init(usage, size,
              vk::MemoryPropertyFlagBits::eHostVisible |
                  vk::MemoryPropertyFlagBits::eHostCoherent);
}

bool Buffer::Init(vk::BufferUsageFlags usage, vk::DeviceSize size,
                  vk::MemoryPropertyFlags memoryFlags) {
  usage_ = usage;
  size_ = size;
  memory_flags_ = memoryFlags;

  auto& device = renderer()->device();

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

  auto found = renderer()->FindMemoryType(req.memoryTypeBits, memory_flags_,
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

bool Buffer::SetData(void* data, size_t size) {
  auto& device = renderer()->device();

  vk::Result result = vk::Result::eSuccess;

  void*  mapData = nullptr;
  size_t limitSize = std::min(size, size_);
  result =
      device.mapMemory(memory_, 0, limitSize, vk::MemoryMapFlags(), &mapData);
  if (result != vk::Result::eSuccess) {
    return false;
  }
  memcpy(mapData, data, limitSize);
  device.unmapMemory(memory_);
  return true;
}
}  // namespace impl
}  // namespace VPP
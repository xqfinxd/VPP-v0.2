#include "buffer.h"
#include "device.h"

void BufferObject::destroy(vk::Device& device) {
  if (buffer) {
    device.destroy(buffer);
  }
  if (memccpy) {
    device.free(memory);
  }
}

std::unique_ptr<BufferObject> BufferObject::createFromData(
    vk::BufferUsageFlags usage, void* data, size_t size) {
  auto object = std::make_unique<BufferObject>();
  auto& renderer = Renderer::getMe();
  vk::Result result;
  do {
    auto bufferCI = vk::BufferCreateInfo().setSize(size).setUsage(usage);

    auto result =
        renderer.getDevice().createBuffer(&bufferCI, nullptr, &object->buffer);
    if (result != vk::Result::eSuccess) {
      break;
    }

    vk::MemoryRequirements memReq;
    renderer.getDevice().getBufferMemoryRequirements(object->buffer, &memReq);

    auto memAI = vk::MemoryAllocateInfo()
                     .setAllocationSize(memReq.size)
                     .setMemoryTypeIndex(0);

    bool pass =
        renderer.getMemoryType(memReq.memoryTypeBits,
                               vk::MemoryPropertyFlagBits::eHostVisible |
                                   vk::MemoryPropertyFlagBits::eHostCoherent,
                               memAI.memoryTypeIndex);
    if (result != vk::Result::eSuccess) {
      break;
    }

    result =
        renderer.getDevice().allocateMemory(&memAI, nullptr, &object->memory);
    if (result != vk::Result::eSuccess) {
      break;
    }
    if (data && size) {
      void* ptr = nullptr;
      result = renderer.getDevice().mapMemory(object->memory, 0, memReq.size,
                                              vk::MemoryMapFlags(), &ptr);
      if (result != vk::Result::eSuccess) {
        break;
      }

      memcpy(ptr, &data, size);
      renderer.getDevice().unmapMemory(object->memory);
    }
    renderer.getDevice().bindBufferMemory(object->buffer, object->memory, 0);
    return object;
  } while (false);
  object->destroy(renderer.getDevice());
  return nullptr;
}

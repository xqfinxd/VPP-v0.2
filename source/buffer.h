#pragma once

#include <map>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

struct BufferObject {
  vk::Buffer buffer{};
  vk::DeviceMemory memory{};
  size_t size = 0;

  void destroy(vk::Device& device);

  static std::unique_ptr<BufferObject> createFromData(vk::BufferUsageFlags usage, void* data, size_t size);
};

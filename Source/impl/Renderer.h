#pragma once

#include <vulkan/vulkan.hpp>

#include "Window.h"

namespace VPP {
namespace impl {
struct QueueIndices {
  uint32_t graphics = UINT32_MAX;
  uint32_t present = UINT32_MAX;

  bool HasValue() const {
    return graphics != UINT32_MAX && present != UINT32_MAX;
  }

  bool IsSame() const {
    return graphics == present;
  }

  std::vector<uint32_t> Pack() const {
    std::vector<uint32_t> indices{};
    indices.push_back(graphics);
    if (IsSame()) {
      indices.push_back(present);
    }
    return indices;
  }
};

class Renderer {
 public:
  Renderer(Window& window);
  ~Renderer();

  bool FindMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask,
                      uint32_t& typeIndex) const;

  vk::Instance       instance{};
  vk::SurfaceKHR     surface{};
  vk::PhysicalDevice gpu{};
  vk::Device         device{};
  QueueIndices       indices{};
  struct {
    vk::Queue graphics{};
    vk::Queue present{};
  } queues{};

 private:
  void CreateInstance(SDL_Window* window);
  void CreateSurface(SDL_Window* window);
  void SetGpuAndIndices();
  void CreateDevice();
  void GetQueues();
};
}  // namespace impl
}  // namespace VPP

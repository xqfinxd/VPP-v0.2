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

  const vk::Device& device() const {
    return device_;
  }

  const QueueIndices& indices() const {
    return indices_;
  }

  vk::SurfaceCapabilitiesKHR GetCapabilities() const;
  std::vector<vk::PresentModeKHR> GetPresentModes() const;
  std::vector<vk::SurfaceFormatKHR> GetFormats() const;

  vk::SwapchainKHR CreateSwapchain(vk::SwapchainKHR oldSwapchain, vk::SurfaceFormatKHR format, vk::PresentModeKHR presentMode);

 private:
  vk::Instance       instance_{};
  vk::SurfaceKHR     surface_{};
  vk::PhysicalDevice gpu_{};
  vk::Device         device_{};
  QueueIndices       indices_{};
  vk::Queue          graphics_queues_{};
  vk::Queue          present_queues_{};

 private:
  void CreateInstance(SDL_Window* window);
  void CreateSurface(SDL_Window* window);
  void SetGpuAndIndices();
  void CreateDevice();
  void GetQueues();
};
}  // namespace impl
}  // namespace VPP

#pragma once

#include <vulkan/vulkan.hpp>

#include "UniqueArray.hpp"
#include "Window.h"

namespace VPP {
namespace impl {
struct SwapchainResource {
  uint32_t image_count{};

  uint32_t image_index{};
  uarray<vk::Image> images{};
  uarray<vk::ImageView> imageviews{};

  vk::Image depth_image;
  vk::ImageView depth_imageview;
  vk::DeviceMemory depth_memory;

  vk::RenderPass render_pass{};
  uarray<vk::Framebuffer> framebuffers{};

  vk::CommandPool command_pool{};
  uarray<vk::CommandBuffer> commands{};

  void Destroy(const vk::Device& device);
};

class Device : protected Singleton<Device>, protected std::enable_shared_from_this<Device> {
  friend class DeviceResource;

 public:
  Device(std::shared_ptr<Window> window);
  ~Device();

  bool FindMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask, uint32_t& typeIndex) const;

  void ReCreateSwapchain();

 private:
  vk::Instance instance_{};
  vk::SurfaceKHR surface_{};
  vk::PhysicalDevice gpu_{};
  vk::Device device_{};

  uint32_t graphics_index_{UINT32_MAX};
  uint32_t present_index_{UINT32_MAX};

  vk::Queue graphics_queue_{};
  vk::Queue present_queue_{};

  uint32_t frame_count_{0};
  uint32_t frame_index_{};
  uarray<vk::Fence> fences_{};
  uarray<vk::Semaphore> image_acquired_{};
  uarray<vk::Semaphore> render_complete_{};

  vk::SwapchainKHR swapchain_{};
  SwapchainResource resource_{};

 private:
  void CreateInstance(SDL_Window* window);
  void CreateSurface(SDL_Window* window);
  void SetGpuAndIndices();
  void CreateDevice();
  void GetQueues();
  void CreateSyncObject();
  void CreateSwapchain(vk::SwapchainKHR oldSwapchain);
  void GetSwapchainImages();
  void CreateSwapchainImageViews(vk::Format format);
  void CreateDepthbuffer(vk::Extent2D extent);
  void CreateRenderPass(vk::Format format);
  void CreateFramebuffers(vk::Extent2D extent);
  void CreateCommandBuffers();
};

class DeviceResource {
 protected:
  DeviceResource();

  const vk::Device& device() const {
    return parent_->device_;
  }

  vk::DeviceMemory CreateMemory(vk::MemoryRequirements& req, vk::MemoryPropertyFlags flags) const;

 private:
  std::shared_ptr<Device> parent_;
};
}  // namespace impl
}  // namespace VPP

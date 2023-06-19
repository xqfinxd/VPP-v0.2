#pragma once

#include <vulkan/vulkan.hpp>

#include "UniqueArray.hpp"
#include "Window.h"

namespace VPP {
namespace impl {

class Device : protected Singleton<Device>,
               public std::enable_shared_from_this<Device> {
  friend class DeviceResource;

public:
  Device(std::shared_ptr<Window> window);
  ~Device();

  void ReCreateSwapchain();

  uint32_t GetDrawCount() const {
    return swapchain_image_count_;
  }

  void Draw();
  void EndDraw();

private:
  void CreateInstance(SDL_Window* window);
  void CreateSurface(SDL_Window* window);
  void SetGpuAndIndices();
  void CreateDevice();
  void GetQueues();
  void CreateSyncObject();
  void CreateSwapchain(vk::SwapchainKHR oldSwapchain);
  void DestroySwapchainResource();
  void GetSwapchainImages();
  void CreateSwapchainImageViews(vk::Format format);
  void CreateDepthbuffer(vk::Extent2D extent);
  void CreateRenderPass(vk::Format format);
  void CreateFramebuffers(vk::Extent2D extent);
  void CreateCommandBuffers();
  bool FindMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask,
                      uint32_t& typeIndex) const;

private:
  vk::Instance instance_{};
  vk::SurfaceKHR surface_{};
  vk::PhysicalDevice gpu_{};
  vk::Device device_{};
  vk::PhysicalDeviceProperties property_{};

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
  vk::Extent2D extent_{};

  uint32_t swapchain_image_count_{};

  uint32_t current_buffer_{};
  uarray<vk::Image> swapchain_images_{};
  uarray<vk::ImageView> swapchain_imageviews_{};

  vk::Image depth_image_{};
  vk::ImageView depth_imageview_{};
  vk::DeviceMemory depth_memory_{};

  vk::RenderPass render_pass_{};
  uarray<vk::Framebuffer> framebuffers_{};

  vk::CommandPool command_pool_{};
  uarray<vk::CommandBuffer> commands_{};
};

class DeviceResource {
protected:
  DeviceResource();

  const vk::Device& device() const {
    return parent_->device_;
  }

  const vk::RenderPass& render_pass() const {
    return parent_->render_pass_;
  }

  const vk::Framebuffer& framebuffer(uint32_t index) const {
    return parent_->framebuffers_[index];
  }

  const vk::Framebuffer& framebuffer() const {
    return framebuffer(parent_->current_buffer_);
  }

  const vk::CommandBuffer& command(uint32_t index) const {
    return parent_->commands_[index];
  }
  const vk::CommandBuffer& command() const {
    return command(parent_->current_buffer_);
  }

  const vk::Extent2D& surface_extent() const {
    return parent_->extent_;
  }

  vk::DeviceMemory CreateMemory(vk::MemoryRequirements& req,
                                vk::MemoryPropertyFlags flags) const;

private:
  std::shared_ptr<Device> parent_;
};

} // namespace impl
} // namespace VPP

#pragma once

#include <vulkan/vulkan.hpp>

#include "Window.h"

namespace VPP {
namespace impl {

class DrawParam;

class Device {
  friend class DeviceResource;

public:
  Device(Window* window);
  ~Device();

  void ReCreateSwapchain();

  uint32_t GetDrawCount() const { return swapchain_image_count_; }

  void set_cmd(const DrawParam& cmd);
  void Draw();
  void EndDraw();

private:
  void CreateInstance(SDL_Window* window);
  void CreateSurface(SDL_Window* window);
  void SetGpuAndIndices();
  void CreateDevice();
  void GetQueues();
  void CreateSyncObject();
  void CreateSwapchainResource(vk::SwapchainKHR oldSwapchain);
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
  std::unique_ptr<vk::Fence[]> fences_{};
  std::unique_ptr<vk::Semaphore[]> image_acquired_{};
  std::unique_ptr<vk::Semaphore[]> render_complete_{};

  vk::SwapchainKHR swapchain_{};
  vk::Extent2D extent_{};

  uint32_t swapchain_image_count_{};

  uint32_t current_buffer_{};
  std::unique_ptr<vk::Image[]> swapchain_images_{};
  std::unique_ptr<vk::ImageView[]> swapchain_imageviews_{};

  vk::Image color_image_{};
  vk::ImageView color_imageview_{};
  vk::DeviceMemory color_memory_{};

  vk::Image depth_image_{};
  vk::ImageView depth_imageview_{};
  vk::DeviceMemory depth_memory_{};

  vk::RenderPass render_pass_{};
  vk::Framebuffer framebuffers_{};

  vk::CommandPool command_pool_{};
  std::unique_ptr<vk::CommandBuffer[]> commands_{};

  const DrawParam* cmd_;
};

class DeviceResource {
protected:
  DeviceResource();
  ~DeviceResource();

  const vk::Device& device() const { return parent_->device_; }
  const vk::PhysicalDevice& gpu() const { return parent_->gpu_; }
  const vk::RenderPass& render_pass() const { return parent_->render_pass_; }
  const vk::Extent2D& surface_extent() const { return parent_->extent_; }
  vk::DeviceMemory CreateMemory(const vk::MemoryRequirements& req,
                                vk::MemoryPropertyFlags flags) const;
  vk::Buffer CreateBuffer(vk::BufferUsageFlags flags, size_t size) const;
  bool CopyBuffer2Buffer(const vk::Buffer& srcBuffer,
                         const vk::Buffer& dstBuffer, size_t size) const;
  bool CopyBuffer2Image(const vk::Buffer& srcBuffer, const vk::Image& dstBuffer,
                        uint32_t width, uint32_t height,
                        uint32_t channel) const;

  bool CopyPresent(const vk::Image& to) const;

private:
  vk::CommandBuffer BeginOnceCmd() const;
  void EndOnceCmd(vk::CommandBuffer& cmd) const;
  void SetImageForTransfer(const vk::CommandBuffer& cmd,
                           const vk::Image& image) const;
  void SetImageForShader(const vk::CommandBuffer& cmd,
                         const vk::Image& image) const;

private:
  Device* parent_ = nullptr;
};

class StageBuffer : public DeviceResource {
public:
  StageBuffer(const void* data, size_t size);
  ~StageBuffer();
  bool CopyTo(const vk::Buffer& dstBuffer);
  bool CopyTo(const vk::Image& dstImage, uint32_t width, uint32_t height,
              uint32_t channel);

private:
  size_t size_ = 0;
  vk::Buffer buffer_{};
  vk::DeviceMemory memory_{};
};

Device* GetDevice();

} // namespace impl
} // namespace VPP

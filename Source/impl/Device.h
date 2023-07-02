#pragma once

#include <vulkan/vulkan.hpp>

#include "Window.h"

namespace VPP {
namespace impl {

class DrawParam;

struct DeviceOption {
  bool enable_debug = true;
};

struct QueueReference {
  uint32_t  index = UINT32_MAX;
  vk::Queue queue;
};

struct SwapchainObject {
  vk::SwapchainKHR       object;
  vk::Format             format;
  vk::Extent2D           extent;
  vk::PresentModeKHR     present_mode;
  vk::ImageUsageFlags    usage;
  std::vector<vk::Image> images;

  uint32_t ImageCount() const { return (uint32_t)images.size(); }

  operator bool() const { return bool(object); }
};

struct DepthBufferObject {
  vk::Image        image{};
  vk::DeviceMemory memory{};
  vk::Format       format;
  vk::Extent2D     extent;
};

class Device {
  friend class DeviceResource;

public:
  Device(Window* window, const DeviceOption& options);
  ~Device();

  void ReCreateSwapchain();

  void Draw();
  void EndDraw();

private:
  void CreateInstanceAndSurface(SDL_Window* window, bool enableDbg);
  void SetGpuAndIndices();
  void CreateDeviceAndQueue(bool enableDbg);
  void CreateCommandPools();

  bool CreateSwapchainObject(SwapchainObject&    swapchain,
                             vk::ImageUsageFlags usages);
  void CreateSyncObject();
  void CreateDepthbuffer(vk::Extent2D extent);
  void DestroyDepthbuffer();
  void DestroySwapchainObject(SwapchainObject& swapchain);

  bool FindMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask,
                      uint32_t& typeIndex) const;

private:
  Window* window_;

  vk::Instance                 instance_{};
  vk::SurfaceKHR               surface_{};
  vk::PhysicalDevice           gpu_{};
  vk::Device                   device_{};
  vk::PhysicalDeviceProperties property_{};

  QueueReference graphics_;
  QueueReference transfer_;
  QueueReference present_;

  std::unique_ptr<vk::Fence[]>     fences_{};
  std::unique_ptr<vk::Semaphore[]> image_acquired_{};
  std::unique_ptr<vk::Semaphore[]> render_complete_{};

  SwapchainObject                  swapchain_;
  DepthBufferObject                depth_;

  uint32_t current_buffer_{};

  vk::CommandPool reset_command_pool_{};
  vk::CommandPool once_command_pool_{};
};

class StageBuffer : public DeviceResource {
public:
  StageBuffer(Device* parent, const void* data, size_t size);
  ~StageBuffer();
  bool CopyToBuffer(const vk::Buffer& dstBuffer, size_t size);
  bool CopyToImage(const vk::Image& dstImage, uint32_t width, uint32_t height,
                   uint32_t channel);

  operator bool() const { return buffer_ && memory_; }

private:
  vk::Buffer       buffer_{};
  vk::DeviceMemory memory_{};
};

class DeviceResource {
protected:
  DeviceResource(Device* device);
  ~DeviceResource();

  const vk::Device&         device() const { return parent_->device_; }
  
  vk::DeviceMemory CreateMemory(const vk::MemoryRequirements& req,
                                vk::MemoryPropertyFlags       flags) const;
  vk::Buffer       CreateBuffer(vk::BufferUsageFlags flags, size_t size) const;

  bool CopyPresent(const vk::Image& to) const;

  std::unique_ptr<StageBuffer> CreateStageBuffer(const void* data,
                                                 size_t      size) {
    auto stageBuffer = std::make_unique<StageBuffer>(parent_, data, size);
    if (!*stageBuffer) {
      return nullptr;
    }
    return stageBuffer;
  }

  bool CheckFormatTilingOptimal(vk::Format             format,
                                vk::FormatFeatureFlags flags) {
    auto prop = parent_->gpu_.getFormatProperties(format);
    return (prop.optimalTilingFeatures & flags) != (vk::FormatFeatureFlags)0;
  }

  vk::CommandBuffer BeginOnceCmd() const;
  void EndOnceCmd(vk::CommandBuffer& cmd) const;

private:
  Device* parent_ = nullptr;
};

} // namespace impl
} // namespace VPP

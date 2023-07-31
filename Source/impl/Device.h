#pragma once

#include <vulkan/vulkan.hpp>

#include "Window.h"

namespace VPP {
namespace impl {

class DrawParam;
class RenderPath;

class Device {
  friend class DeviceResource;

public:
  Device(Window* window);
  ~Device();

  void ReCreateSwapchain();

  void InitRenderPath(RenderPath* path);
  void PrepareRender();
  void Render(DrawParam* param);
  void FinishRender();

  void EndDraw();

private:
  void CreateInstance(SDL_Window* window);
  void CreateSurface(SDL_Window* window);
  void SelectPhysicalDevice();
  void SelectQueueIndex();
  void CreateDevice();
  void GetQueues();
  void CreateSyncObject();
  void CreateSwapchainResource(vk::SwapchainKHR oldSwapchain);
  void DestroySwapchainResource();
  void CreateSwapchainImageViews(vk::Format format);
  void CreateDepthbuffer(vk::Extent2D extent);
  void CreateCommandBuffers();
  bool FindMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask,
                      uint32_t& typeIndex) const;

private:
  vk::Instance instance_{};
  vk::SurfaceKHR surface_{};
  vk::PhysicalDevice gpu_{};
  vk::Device device_{};
  vk::PhysicalDeviceProperties property_{};

  struct {
    uint32_t graphics = UINT32_MAX;
    uint32_t present = UINT32_MAX;
  } indices_{};

  struct {
    vk::Queue graphics;
    vk::Queue present;
  } queues_{};

  uint32_t frame_count_{0};
  uint32_t frame_index_{};
  std::unique_ptr<vk::Fence[]> fences_{};
  std::unique_ptr<vk::Semaphore[]> image_acquired_{};
  std::unique_ptr<vk::Semaphore[]> render_complete_{};

  struct {
    vk::SwapchainKHR handle;
    vk::Extent2D image_extent;
    vk::Format image_format = vk::Format::eUndefined;
  } swapchain_{};

  struct {
    uint32_t count = 0;
    std::unique_ptr<vk::Image[]> images;
    std::unique_ptr<vk::ImageView[]> imageviews;
  } swapbuffers_{};

  uint32_t current_buffer_{};
  
  struct {
    vk::Format format = vk::Format::eUndefined;
    vk::Image image{};
    vk::ImageView imageview{};
    vk::DeviceMemory memory{};
  } depth_{};

  vk::CommandPool command_pool_{};
  std::unique_ptr<vk::CommandBuffer[]> commands_{};
};

class DeviceResource {
public:
  Device* GetParent() { return parent_; }
  const Device* GetParent() const {
    return parent_;
  }

protected:
  DeviceResource(Device* pParent);
  ~DeviceResource();
  
  const vk::Device& device() const { return parent_->device_; }

  vk::DeviceMemory CreateMemory(const vk::MemoryRequirements& req,
                                vk::MemoryPropertyFlags flags) const;
  vk::Buffer CreateBuffer(vk::BufferUsageFlags flags, size_t size) const;
  bool CopyBuffer2Buffer(const vk::Buffer& srcBuffer,
                         const vk::Buffer& dstBuffer, size_t size) const;
  bool CopyBuffer2Image(const vk::Buffer& srcBuffer, const vk::Image& dstBuffer,
                        uint32_t width, uint32_t height,
                        uint32_t channel) const;

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
  StageBuffer(DeviceResource* dst, const void* data, size_t size);
  ~StageBuffer();
  bool CopyToBuffer(const vk::Buffer& dstBuffer);
  bool CopyToImage(const vk::Image& dstImage, uint32_t width, uint32_t height,
              uint32_t channel);

private:
  size_t size_ = 0;
  vk::Buffer buffer_{};
  vk::DeviceMemory memory_{};
};

} // namespace impl
} // namespace VPP

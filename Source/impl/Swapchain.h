#pragma once

#include <vulkan/vulkan.hpp>

#include "Renderer.h"
#include "UniqueArray.hpp"

namespace VPP {
namespace impl {
class Swapchain {
  friend class Device;

  struct Depthbuffer {
    vk::Image        image;
    vk::ImageView    view;
    vk::DeviceMemory memory;
  };

 public:
  Swapchain(Renderer* renderer);
  ~Swapchain();

  void ReCreate(Renderer* renderer);

 private:
  Renderer*        renderer_ = nullptr;
  vk::SwapchainKHR swapchain_{};

  uint32_t             swapchain_image_count_{};
  Array<vk::Image>     swapchain_images_{};
  Array<vk::ImageView> swapchain_imageviews_{};
  Depthbuffer          depthbuffer_;

  vk::RenderPass         render_pass_{};
  Array<vk::Framebuffer> framebuffers_{};

  vk::CommandPool          command_pool_{};
  Array<vk::CommandBuffer> commands_{};

  uint32_t image_index_{};

  bool           enable_clear_{};
  vk::ClearValue clear_value_{};

  uint32_t             frame_index_{};
  Array<vk::Fence>     device_idle_{};
  Array<vk::Semaphore> image_acquired_{};
  Array<vk::Semaphore> render_complete_{};

 private:
  void Create();
  void Destroy();

  void GetSwapchainImages();
  void CreateSwapchainImageViews(vk::Format format);
  void CreateDepthbuffer(vk::Extent2D extent);
  void CreateRenderPass(vk::Format format);
  void CreateFramebuffers(vk::Extent2D extent);
  void CreateCommandBuffers();
  void CreateSyncObject();
};
}  // namespace impl
}  // namespace VPP

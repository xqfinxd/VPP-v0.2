#pragma once

#include <vulkan/vulkan.hpp>

#include "Renderer.h"
#include "UniqueArray.hpp"

namespace VPP {
namespace impl {
class Swapchain {
  struct Depthbuffer {
    vk::Image        image;
    vk::ImageView    view;
    vk::DeviceMemory memory;
  };

  struct InitializeInfo {
    uint32_t             width{};
    uint32_t             height{};
    vk::SurfaceFormatKHR surface_format{};
    vk::PresentModeKHR   present_mode{};
    uint32_t             min_image_count;
  };

 public:
  Swapchain(Renderer* renderer);
  ~Swapchain();

  void ReCreate(Renderer* renderer);

  InitializeInfo info{};

  Renderer*            renderer;
  vk::SwapchainKHR     swapchain{};

  vk::SurfaceCapabilitiesKHR capabilities{};
  vk::SurfaceFormatKHR       surface_format{};
  vk::PresentModeKHR         present_mode{};
  uint32_t             swapchain_image_count{};
  Array<vk::Image>     swapchain_images{};
  Array<vk::ImageView> swapchain_imageviews{};
  Depthbuffer          depthbuffer;

  vk::RenderPass         render_pass{};
  Array<vk::Framebuffer> framebuffers{};

  vk::CommandPool          command_pool{};
  Array<vk::CommandBuffer> commands{};

  uint32_t image_index{};

  bool           enable_clear{};
  vk::ClearValue clear_value{};

  uint32_t             frame_index{};
  Array<vk::Fence>     device_idle{};
  Array<vk::Semaphore> image_acquired{};
  Array<vk::Semaphore> render_complete{};

 private:
  void Create();
  void Destroy(bool excludeSwapchain);

  void UpdateInfo();
  void CreateSwapchain(vk::SwapchainKHR oldSwapchain);
  void CreateSwapchainImageViews();
  void CreateDepthbuffer();
  void CreateRenderPass();
  void CreateFramebuffers();
  void CreateCommandBuffers();
  void CreateSyncObject();
};
}  // namespace impl
}  // namespace VPP

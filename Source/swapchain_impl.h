#pragma once

#include <vulkan/vulkan.hpp>

#include "public/singleton.h"
#include "unique_array.h"

namespace VPP {

namespace impl {

class Swapchain : public Singleton<Swapchain> {
  struct Depthbuffer {
    vk::Image        image;
    vk::ImageView    view;
    vk::DeviceMemory memory;
  };

  struct InitialzeInfo {
    uint32_t             width{};
    uint32_t             height{};
    vk::SurfaceFormatKHR surface_format{};
    vk::PresentModeKHR   present_mode{};
    uint32_t             min_image_count;
  };

 public:
  InitialzeInfo info{};

  vk::SwapchainKHR     swapchain{};
  uint32_t             image_count{};
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

  void Init();
  void Quit();

 private:
  void InitInfo();
  void CreateSwapchain();
  void CreateSwapchainImageViews();
  void CreateDepthbuffer();
  void CreateRenderPass();
  void CreateFramebuffers();
  void CreateCommandBuffers();
  void CreateSyncObject();
};

}  // namespace impl

}  // namespace VPP

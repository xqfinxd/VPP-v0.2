#pragma once

#include <vulkan/vulkan.hpp>

#include "public/singleton.h"

namespace VPP {

namespace impl {

struct QueueIndices {
  uint32_t graphics = UINT32_MAX;
  uint32_t present = UINT32_MAX;

  bool HasValue() const {
    return graphics != UINT32_MAX && present != UINT32_MAX;
  }

  std::vector<uint32_t> Pack() const {
    std::vector<uint32_t> indices{};
    indices.push_back(graphics);
    if (graphics != present) {
      indices.push_back(present);
    }
    return indices;
  }
};

struct DeviceQueues {
  vk::Queue graphics{};
  vk::Queue present{};
};

struct DepthBuffer {
  vk::Image        image{};
  vk::ImageView    view{};
  vk::DeviceMemory memory{};
};

struct RenderFrame {
  vk::CommandBuffer command_buffer{};
  vk::Fence         fence{};
  vk::Image         colorbuffer{};
  vk::ImageView     colorbuffer_view{};
  vk::Framebuffer   framebuffer{};
};

struct FrameSemaphores {
  vk::Semaphore image_acquired;
  vk::Semaphore render_complete;
};

struct RenderContext {
  //  set when surface created
  uint32_t             width{};
  uint32_t             height{};
  vk::SurfaceFormatKHR surface_format{};
  vk::PresentModeKHR   present_mode{};
  uint32_t             min_image_count;

  // set after device created
  vk::SwapchainKHR swapchain{};
  vk::Image        depthbuffer;
  vk::ImageView    depthbuffer_view;
  vk::DeviceMemory depthbuffer_memory;
  vk::RenderPass   render_pass{};
  vk::CommandPool  command_pool{};

  bool           enable_clear_buffer{};
  vk::ClearValue clear_value{};

  uint32_t image_count{};
  uint32_t frame_index{};
  uint32_t semaphore_index{};

  std::unique_ptr<RenderFrame[]>     frames{};
  std::unique_ptr<FrameSemaphores[]> semaphores{};
};

class Renderer : public Singleton<Renderer> {
 public:
  Renderer();
  ~Renderer();

  void Setup();
  void Clean();

  bool FindMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask,
                      uint32_t& typeIndex) const;

  const vk::Device& device() const {
    return device_;
  }

 private:
  vk::Instance       instance_{};
  vk::SurfaceKHR     surface_{};
  vk::PhysicalDevice gpu_{};
  vk::Device         device_{};
  QueueIndices       indices_{};
  DeviceQueues       queues_{};

  RenderContext context{};

  void SetupRenderer();
  void CleanRenderer();
  void SetupContext(RenderContext& ctx);
  void CleanContext(RenderContext& ctx);

  void CreateFrame(const RenderContext& ctx, RenderFrame& frame);
  void DestroyFrame(RenderFrame& frame);
  void CreateSemaphore(const RenderContext& ctx, FrameSemaphores& semaphore);
  void DestroySemaphore(FrameSemaphores& frame);
};

}  // namespace impl

}  // namespace VPP

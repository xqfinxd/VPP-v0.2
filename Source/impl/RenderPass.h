#pragma once

#include <vulkan/vulkan.hpp>

#include <map>

#include "Buffer.h"
#include "Device.h"
#include "Image.h"
#include "Program.h"

namespace VPP {

namespace impl {

class RenderPass {
public:
  virtual ~RenderPass() {}

  virtual bool Init(const SwapchainObject&   swapchain,
                    const DepthBufferObject& depth) {
    return InitRenderPass(swapchain.format, depth.format) &&
           InitFramebuffers(swapchain.extent, swapchain.ImageCount(),
                            swapchain.images.data(), depth.image);
  }

  virtual const vk::RenderPass& renderpass() const { return GetRenderPass(); }

protected:
  virtual bool InitRenderPass(vk::Format colorFormat,
                              vk::Format depthFormat)                 = 0;
  virtual bool InitFramebuffers(vk::Extent2D extent, uint32_t imageCount,
                                const vk::Image* colorImages,
                                vk::Image        depthImage)                 = 0;
  virtual const vk::Framebuffer& GetFramebuffer(uint32_t index) const = 0;
  virtual const vk::RenderPass&  GetRenderPass() const                = 0;

  virtual std::vector<vk::CommandBuffer>
  DrawCommand(const vk::RenderPass&  renderpass,
              const vk::Framebuffer& framebuffer, const vk::Extent2D& area) = 0;

  vk::Extent2D surface_extent_;
};

class GeneralRenderPass : public RenderPass, public DeviceResource {
public:
  GeneralRenderPass(Device* parent);
  ~GeneralRenderPass();

protected:
private:
  std::vector<vk::Framebuffer> framebuffers_;
};

} // namespace impl

} // namespace VPP

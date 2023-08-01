#pragma once

#include <vulkan/vulkan.hpp>

#include "Window.h"

namespace VPP {
namespace impl {

class DrawParam;
class ShaderPass;

class Device {
  friend class DeviceResource;

public:
  Device(Window* window);
  ~Device();

  void ReCreateSwapchain();

  void InitRenderPath(ShaderPass* path);
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
  vk::CommandPool once_cmd_pool_;
  std::unique_ptr<vk::CommandBuffer[]> commands_{};
};

class DeviceResource {
public:
  Device* GetParent() {
    return parent_;
  }
  const Device* GetParent() const {
    return parent_;
  }

protected:
  DeviceResource(Device* pParent);
  ~DeviceResource();

  const vk::Device& device() const {
    return parent_->device_;
  }
  uint32_t queue_index() const {
    return parent_->indices_.graphics;
  }
  const vk::Queue& queue() const {
    return parent_->queues_.graphics;
  }
  const vk::CommandPool& once_cmd_pool() const {
    return parent_->once_cmd_pool_;
  }

  vk::DeviceMemory CreateMemory(const vk::MemoryRequirements& req,
                                vk::MemoryPropertyFlags flags) const;
  vk::Buffer CreateBuffer(vk::BufferUsageFlags flags, size_t size) const;
  
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

class CommandOnce : public DeviceResource {
public:
  CommandOnce(DeviceResource* dst) : DeviceResource(dst->GetParent()) {
    auto cmdAI = vk::CommandBufferAllocateInfo()
                     .setCommandPool(once_cmd_pool())
                     .setLevel(vk::CommandBufferLevel::ePrimary)
                     .setCommandBufferCount(1);

    auto result = device().allocateCommandBuffers(&cmdAI, &primary_);
    if (result == vk::Result::eSuccess) {
      auto beginInfo = vk::CommandBufferBeginInfo().setFlags(
          vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
      primary_.begin(beginInfo);
    }
  }

  operator bool() const {
    return static_cast<bool>(primary_);
  }

  const vk::CommandBuffer& Get() const {
    return primary_;
  }

  void CopyBuffer2Buffer(const vk::Buffer& srcBuffer,
                         const vk::Buffer& dstBuffer, size_t size) const {
    if (!primary_) return;

    auto copyRegion =
        vk::BufferCopy().setDstOffset(0).setSrcOffset(0).setSize(size);
    primary_.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
  }

  void SetImageForCopyDst(const vk::Image& image) const {
    if (!primary_) return;

    auto barrier = vk::ImageMemoryBarrier()
                       .setOldLayout(vk::ImageLayout::eUndefined)
                       .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                       .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setImage(image)
                       .setSubresourceRange(vk::ImageSubresourceRange{
                           vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
                       .setSrcAccessMask((vk::AccessFlags)0)
                       .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
    primary_.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                             vk::PipelineStageFlagBits::eTransfer,
                             (vk::DependencyFlagBits)0, 0, nullptr, 0, nullptr,
                             1, &barrier);
  }

  void SetImageForSample(const vk::Image& image) const {
    if (!primary_) return;

    auto barrier = vk::ImageMemoryBarrier()
                       .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                       .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                       .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setImage(image)
                       .setSubresourceRange(vk::ImageSubresourceRange{
                           vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
                       .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                       .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
    primary_.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                             vk::PipelineStageFlagBits::eFragmentShader |
                                 vk::PipelineStageFlagBits::eVertexShader,
                             (vk::DependencyFlagBits)0, 0, nullptr, 0, nullptr,
                             1, &barrier);
  }

  void CopyBuffer2Image(const vk::Buffer& srcBuffer, const vk::Image& dstImage,
                        uint32_t width, uint32_t height, uint32_t comp) const {
    if (!primary_) return;

    SetImageForCopyDst(dstImage);
    auto region = vk::BufferImageCopy()
                      .setBufferOffset(0)
                      .setBufferRowLength(width)
                      .setBufferImageHeight(height)
                      .setImageSubresource(vk::ImageSubresourceLayers{
                          vk::ImageAspectFlagBits::eColor, 0, 0, 1})
                      .setImageOffset(vk::Offset3D{0, 0, 0})
                      .setImageExtent(vk::Extent3D{width, height, 1});
    primary_.copyBufferToImage(
        srcBuffer, dstImage, vk::ImageLayout::eTransferDstOptimal, 1, &region);
    SetImageForSample(dstImage);
  }

  ~CommandOnce() {
    if (primary_) {
      primary_.end();

      queue().waitIdle();
      auto submitInfo =
          vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(
              &primary_);
      queue().submit(1, &submitInfo, VK_NULL_HANDLE);
      queue().waitIdle();

      device().free(once_cmd_pool(), 1, &primary_);
      primary_ = VK_NULL_HANDLE;
    }
  }

private:
  vk::CommandBuffer primary_;
};

} // namespace impl
} // namespace VPP

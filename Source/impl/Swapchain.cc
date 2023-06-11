#include "Swapchain.h"

#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <set>

#include "Variables.h"

namespace VPP {
namespace impl {
Swapchain::Swapchain(Renderer* renderer) : renderer_(renderer) {
  assert(renderer_);
  Create();
}

Swapchain::~Swapchain() {
  Destroy();

  if (swapchain_) {
    renderer_->device().destroy(swapchain_);
  }
}

void Swapchain::ReCreate(Renderer* renderer) {
  Destroy();

  auto capabilities = renderer->GetCapabilities();
  auto surface_format = renderer->GetFormats()[0];
  auto present_mode = renderer->GetPresentModes()[0];

  auto oldSwapchain = swapchain_;
  swapchain_ =
      renderer->CreateSwapchain(oldSwapchain, surface_format, present_mode);
  if (oldSwapchain) {
    renderer->device().destroy(oldSwapchain);
  }

  GetSwapchainImages();
  CreateSwapchainImageViews(surface_format.format);
  CreateDepthbuffer(capabilities.currentExtent);
  CreateRenderPass(surface_format.format);
  CreateFramebuffers(capabilities.currentExtent);
  CreateCommandBuffers();
  CreateSyncObject();
}

void Swapchain::Create() {
  auto capabilities = renderer_->GetCapabilities();
  auto surface_format = renderer_->GetFormats()[0];
  auto present_mode = renderer_->GetPresentModes()[0];

  swapchain_ =
      renderer_->CreateSwapchain(swapchain_, surface_format, present_mode);

  assert(swapchain_);
  GetSwapchainImages();
  CreateSwapchainImageViews(surface_format.format);
  CreateDepthbuffer(capabilities.currentExtent);
  CreateRenderPass(surface_format.format);
  CreateFramebuffers(capabilities.currentExtent);
  CreateCommandBuffers();
  CreateSyncObject();
}

void Swapchain::Destroy() {
  auto& device = renderer_->device();

  for (uint32_t i = 0; i < swapchain_image_count_; ++i) {
    if (device_idle_[i]) {
      device.destroy(device_idle_[i]);
    }

    if (image_acquired_[i]) {
      device.destroy(image_acquired_[i]);
    }

    if (render_complete_[i]) {
      device.destroy(render_complete_[i]);
    }

    if (framebuffers_[i]) {
      device.destroy(framebuffers_[i]);
    }

    if (swapchain_imageviews_[i]) {
      device.destroy(swapchain_imageviews_[i]);
    }
  }
  if (command_pool_) {
    device.destroy(command_pool_);
  }

  if (render_pass_) {
    device.destroy(render_pass_);
  }

  if (depthbuffer_.image) {
    device.destroy(depthbuffer_.image);
  }

  if (depthbuffer_.view) {
    device.destroy(depthbuffer_.view);
  }

  if (depthbuffer_.memory) {
    device.free(depthbuffer_.memory);
  }
}

void Swapchain::GetSwapchainImages() {
  vk::Result result = vk::Result::eSuccess;

  result = renderer_->device().getSwapchainImagesKHR(
      swapchain_, &swapchain_image_count_, nullptr);
  assert(result == vk::Result::eSuccess);
  swapchain_images_ = NewArray<vk::Image>(swapchain_image_count_);
  result = renderer_->device().getSwapchainImagesKHR(
      swapchain_, &swapchain_image_count_, swapchain_images_.get());
  assert(result == vk::Result::eSuccess);
}

void Swapchain::CreateSwapchainImageViews(vk::Format format) {
  vk::Result result = vk::Result::eSuccess;

  auto resRange = vk::ImageSubresourceRange()
                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                      .setLayerCount(1)
                      .setBaseArrayLayer(0)
                      .setLevelCount(1)
                      .setBaseMipLevel(0);

  swapchain_imageviews_ = NewArray<vk::ImageView>(swapchain_image_count_);

  for (uint32_t i = 0; i < swapchain_image_count_; i++) {
    vk::ImageViewCreateInfo imageViewCI =
        vk::ImageViewCreateInfo()
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(format)
            .setPNext(nullptr)
            .setSubresourceRange(resRange);
    imageViewCI.setImage(swapchain_images_[i]);
    result = renderer_->device().createImageView(&imageViewCI, nullptr,
                                                 &swapchain_imageviews_[i]);
    assert(result == vk::Result::eSuccess);
  }
}

void Swapchain::CreateDepthbuffer(vk::Extent2D extent) {
  vk::Result result = vk::Result::eSuccess;

  auto imageCI = vk::ImageCreateInfo()
                     .setFormat(vk::Format::eD16Unorm)
                     .setImageType(vk::ImageType::e2D)
                     .setExtent(vk::Extent3D(extent, 1u))
                     .setMipLevels(1)
                     .setArrayLayers(1)
                     .setSamples(vk::SampleCountFlagBits::e1)
                     .setTiling(vk::ImageTiling::eOptimal)
                     .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
                     .setInitialLayout(vk::ImageLayout::eUndefined);
  if (renderer_->indices().IsSame()) {
    imageCI.setSharingMode(vk::SharingMode::eExclusive);
  } else {
    imageCI.setSharingMode(vk::SharingMode::eConcurrent);
  }
  auto indicesData = renderer_->indices().Pack();
  imageCI.setQueueFamilyIndices(indicesData);
  result =
      renderer_->device().createImage(&imageCI, nullptr, &depthbuffer_.image);
  assert(result == vk::Result::eSuccess);

  vk::MemoryAllocateInfo memoryAI;
  vk::MemoryRequirements memReq;
  renderer_->device().getImageMemoryRequirements(depthbuffer_.image, &memReq);
  memoryAI.setAllocationSize(memReq.size);
  auto pass = renderer_->FindMemoryType(
      memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal,
      memoryAI.memoryTypeIndex);
  assert(pass);
  result = renderer_->device().allocateMemory(&memoryAI, nullptr,
                                              &depthbuffer_.memory);
  assert(result == vk::Result::eSuccess);

  renderer_->device().bindImageMemory(depthbuffer_.image, depthbuffer_.memory,
                                      0);
  auto imageViewCI = vk::ImageViewCreateInfo()
                         .setImage(depthbuffer_.image)
                         .setViewType(vk::ImageViewType::e2D)
                         .setFormat(vk::Format::eD16Unorm)
                         .setSubresourceRange(vk::ImageSubresourceRange(
                             vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1))
                         .setPNext(nullptr);
  result = renderer_->device().createImageView(&imageViewCI, nullptr,
                                               &depthbuffer_.view);
  assert(result == vk::Result::eSuccess);
}

void Swapchain::CreateRenderPass(vk::Format format) {
  vk::Result result = vk::Result::eSuccess;

  std::vector<vk::AttachmentDescription> attachments = {
      vk::AttachmentDescription()
          .setFormat(format)
          .setSamples(vk::SampleCountFlagBits::e1)
          .setLoadOp(vk::AttachmentLoadOp::eClear)
          .setStoreOp(vk::AttachmentStoreOp::eStore)
          .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
          .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setInitialLayout(vk::ImageLayout::eUndefined)
          .setFinalLayout(vk::ImageLayout::ePresentSrcKHR),
      vk::AttachmentDescription()
          .setFormat(vk::Format::eD16Unorm)
          .setSamples(vk::SampleCountFlagBits::e1)
          .setLoadOp(vk::AttachmentLoadOp::eClear)
          .setStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
          .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setInitialLayout(vk::ImageLayout::eUndefined)
          .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)};

  auto colorReference = vk::AttachmentReference().setAttachment(0).setLayout(
      vk::ImageLayout::eColorAttachmentOptimal);

  auto depthReference = vk::AttachmentReference().setAttachment(1).setLayout(
      vk::ImageLayout::eDepthStencilAttachmentOptimal);

  auto subpass = vk::SubpassDescription()
                     .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                     .setInputAttachmentCount(0)
                     .setPInputAttachments(nullptr)
                     .setColorAttachmentCount(1)
                     .setPColorAttachments(&colorReference)
                     .setPResolveAttachments(nullptr)
                     .setPDepthStencilAttachment(&depthReference)
                     .setPreserveAttachmentCount(0)
                     .setPPreserveAttachments(nullptr);

  auto renderPassCI = vk::RenderPassCreateInfo()
                          .setAttachments(attachments)
                          .setSubpassCount(1)
                          .setPSubpasses(&subpass)
                          .setDependencyCount(0)
                          .setPDependencies(nullptr);

  result = renderer_->device().createRenderPass(&renderPassCI, nullptr,
                                                &render_pass_);
  assert(result == vk::Result::eSuccess);
}

void Swapchain::CreateFramebuffers(vk::Extent2D extent) {
  framebuffers_ = NewArray<vk::Framebuffer>(swapchain_image_count_);

  vk::ImageView attachments[2];
  attachments[1] = depthbuffer_.view;

  auto frameBufferCI = vk::FramebufferCreateInfo()
                           .setRenderPass(render_pass_)
                           .setAttachmentCount(2)
                           .setPAttachments(attachments)
                           .setWidth(extent.width)
                           .setHeight(extent.height)
                           .setLayers(1);

  for (uint32_t i = 0; i < swapchain_image_count_; i++) {
    attachments[0] = swapchain_imageviews_[i];
    auto result = renderer_->device().createFramebuffer(&frameBufferCI, nullptr,
                                                        &framebuffers_[i]);
    assert(result == vk::Result::eSuccess);
  }
}

void Swapchain::CreateCommandBuffers() {
  vk::Result result;

  auto cmdPoolCI = vk::CommandPoolCreateInfo().setQueueFamilyIndex(
      renderer_->indices().graphics);
  result = renderer_->device().createCommandPool(&cmdPoolCI, nullptr,
                                                 &command_pool_);
  assert(result == vk::Result::eSuccess);

  auto cmdAI = vk::CommandBufferAllocateInfo()
                   .setCommandPool(command_pool_)
                   .setLevel(vk::CommandBufferLevel::ePrimary)
                   .setCommandBufferCount(1);

  commands_ = NewArray<vk::CommandBuffer>(swapchain_image_count_);
  for (size_t i = 0; i < swapchain_image_count_; i++) {
    result = renderer_->device().allocateCommandBuffers(&cmdAI, &commands_[i]);
    assert(result == vk::Result::eSuccess);
  }
}

void Swapchain::CreateSyncObject() {
  vk::Result result;

  const uint32_t frame_count = g_Vars.frame_lag;

  auto semaphoreCreateInfo = vk::SemaphoreCreateInfo();
  auto fenceCI =
      vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);

  device_idle_ = NewArray<vk::Fence>(frame_count);
  image_acquired_ = NewArray<vk::Semaphore>(frame_count);
  render_complete_ = NewArray<vk::Semaphore>(frame_count);

  for (uint32_t i = 0; i < frame_count; i++) {
    result =
        renderer_->device().createFence(&fenceCI, nullptr, &device_idle_[i]);
    assert(result == vk::Result::eSuccess);

    result = renderer_->device().createSemaphore(&semaphoreCreateInfo, nullptr,
                                                 &image_acquired_[i]);
    assert(result == vk::Result::eSuccess);

    result = renderer_->device().createSemaphore(&semaphoreCreateInfo, nullptr,
                                                 &render_complete_[i]);
    assert(result == vk::Result::eSuccess);
  }
  frame_index_ = 0;
}
}  // namespace impl
}  // namespace VPP
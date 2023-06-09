#include "Swapchain.h"

#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <set>

#include "Variables.h"

namespace VPP {
namespace impl {
Swapchain::Swapchain(Renderer* rnder):renderer(rnder) {
  Create();
}

Swapchain::~Swapchain() {
  Destroy(false);
}

void Swapchain::ReCreate(Renderer* renderer) {
  Destroy(true);
  UpdateInfo();
  CreateSwapchain(swapchain);
  CreateSwapchainImageViews();
  CreateDepthbuffer();
  CreateRenderPass();
  CreateFramebuffers();
  CreateCommandBuffers();
  CreateSyncObject();
}

void Swapchain::Create() {
  UpdateInfo();
  CreateSwapchain(VK_NULL_HANDLE);
  CreateSwapchainImageViews();
  CreateDepthbuffer();
  CreateRenderPass();
  CreateFramebuffers();
  CreateCommandBuffers();
  CreateSyncObject();
}

void Swapchain::Destroy(bool excludeSwapchain) {
  auto& device = renderer->device();

  for (uint32_t i = 0; i < swapchain_image_count; ++i) {
    if (device_idle[i]) {
      device.destroy(device_idle[i]);
    }

    if (image_acquired[i]) {
      device.destroy(image_acquired[i]);
    }

    if (render_complete[i]) {
      device.destroy(render_complete[i]);
    }

    if (framebuffers[i]) {
      device.destroy(framebuffers[i]);
    }

    if (swapchain_imageviews[i]) {
      device.destroy(swapchain_imageviews[i]);
    }
  }
  if (command_pool) {
    device.destroy(command_pool);
  }

  if (render_pass) {
    device.destroy(render_pass);
  }

  if (depthbuffer.image) {
    device.destroy(depthbuffer.image);
  }

  if (depthbuffer.view) {
    device.destroy(depthbuffer.view);
  }

  if (depthbuffer.memory) {
    device.free(depthbuffer.memory);
  }

  if (swapchain && !excludeSwapchain) {
    device.destroy(swapchain);
  }
}

void Swapchain::UpdateInfo() {
  auto capabilities = renderer->GetCapabilities();
  info.width = capabilities.currentExtent.width;
  info.height = capabilities.currentExtent.height;
  info.min_image_count = capabilities.minImageCount;
  auto presentModes = renderer->GetPresentModes();
  info.present_mode = presentModes[0];
  auto surfaceFormats = renderer->GetFormats();
  info.surface_format = surfaceFormats[0];
}

void Swapchain::CreateSwapchain(vk::SwapchainKHR oldSwapchain) {
  capabilities = renderer->GetCapabilities();
  surface_format = renderer->GetFormats()[0];
  present_mode = renderer->GetPresentModes()[0];

  swapchain =
      renderer->CreateSwapchain(swapchain, surface_format, present_mode);
  assert(swapchain);

  vk::Result result = vk::Result::eSuccess;

  result = renderer->device().getSwapchainImagesKHR(
      swapchain, &swapchain_image_count, nullptr);
  assert(result == vk::Result::eSuccess);
  swapchain_images = NewArray<vk::Image>(swapchain_image_count);
  result = renderer->device().getSwapchainImagesKHR(
      swapchain, &swapchain_image_count, swapchain_images.get());
  assert(result == vk::Result::eSuccess);
}

void Swapchain::CreateSwapchainImageViews() {
  vk::Result result = vk::Result::eSuccess;

  auto resRange = vk::ImageSubresourceRange()
                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                      .setLayerCount(1)
                      .setBaseArrayLayer(0)
                      .setLevelCount(1)
                      .setBaseMipLevel(0);

  swapchain_imageviews = NewArray<vk::ImageView>(swapchain_image_count);

  for (uint32_t i = 0; i < swapchain_image_count; i++) {
    vk::ImageViewCreateInfo imageViewCI =
        vk::ImageViewCreateInfo()
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(info.surface_format.format)
            .setPNext(nullptr)
            .setSubresourceRange(resRange);
    imageViewCI.setImage(swapchain_images[i]);
    result = renderer->device().createImageView(&imageViewCI, nullptr,
                                                    &swapchain_imageviews[i]);
    assert(result == vk::Result::eSuccess);
  }
}

void Swapchain::CreateDepthbuffer() {
  vk::Result result = vk::Result::eSuccess;

  auto imageCI = vk::ImageCreateInfo()
                     .setFormat(vk::Format::eD16Unorm)
                     .setImageType(vk::ImageType::e2D)
                     .setExtent(vk::Extent3D(info.width, info.height, 1u))
                     .setMipLevels(1)
                     .setArrayLayers(1)
                     .setSamples(vk::SampleCountFlagBits::e1)
                     .setTiling(vk::ImageTiling::eOptimal)
                     .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
                     .setInitialLayout(vk::ImageLayout::eUndefined);
  if (renderer->indices().IsSame()) {
    imageCI.setSharingMode(vk::SharingMode::eExclusive);
  } else {
    imageCI.setSharingMode(vk::SharingMode::eConcurrent);
  }
  auto indicesData = renderer->indices().Pack();
  imageCI.setQueueFamilyIndices(indicesData);
  result =
      renderer->device().createImage(&imageCI, nullptr, &depthbuffer.image);
  assert(result == vk::Result::eSuccess);

  vk::MemoryAllocateInfo memoryAI;
  vk::MemoryRequirements memReq;
  renderer->device().getImageMemoryRequirements(depthbuffer.image, &memReq);
  memoryAI.setAllocationSize(memReq.size);
  auto pass = renderer->FindMemoryType(
      memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal,
      memoryAI.memoryTypeIndex);
  assert(pass);
  result = renderer->device().allocateMemory(&memoryAI, nullptr,
                                                 &depthbuffer.memory);
  assert(result == vk::Result::eSuccess);

  renderer->device().bindImageMemory(depthbuffer.image, depthbuffer.memory,
                                         0);
  auto imageViewCI = vk::ImageViewCreateInfo()
                         .setImage(depthbuffer.image)
                         .setViewType(vk::ImageViewType::e2D)
                         .setFormat(vk::Format::eD16Unorm)
                         .setSubresourceRange(vk::ImageSubresourceRange(
                             vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1))
                         .setPNext(nullptr);
  result = renderer->device().createImageView(&imageViewCI, nullptr,
                                                  &depthbuffer.view);
  assert(result == vk::Result::eSuccess);
}

void Swapchain::CreateRenderPass() {
  vk::Result result = vk::Result::eSuccess;

  std::vector<vk::AttachmentDescription> attachments = {
      vk::AttachmentDescription()
          .setFormat(info.surface_format.format)
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

  result = renderer->device().createRenderPass(&renderPassCI, nullptr,
                                                   &render_pass);
  assert(result == vk::Result::eSuccess);
}

void Swapchain::CreateFramebuffers() {
  framebuffers = NewArray<vk::Framebuffer>(swapchain_image_count);

  vk::ImageView attachments[2];
  attachments[1] = depthbuffer.view;

  auto frameBufferCI = vk::FramebufferCreateInfo()
                           .setRenderPass(render_pass)
                           .setAttachmentCount(2)
                           .setPAttachments(attachments)
                           .setWidth(info.width)
                           .setHeight(info.height)
                           .setLayers(1);

  for (uint32_t i = 0; i < swapchain_image_count; i++) {
    attachments[0] = swapchain_imageviews[i];
    auto result = renderer->device().createFramebuffer(
        &frameBufferCI, nullptr, &framebuffers[i]);
    assert(result == vk::Result::eSuccess);
  }
}

void Swapchain::CreateCommandBuffers() {
  vk::Result result;

  auto cmdPoolCI = vk::CommandPoolCreateInfo().setQueueFamilyIndex(
      renderer->indices().graphics);
  result = renderer->device().createCommandPool(&cmdPoolCI, nullptr,
                                                    &command_pool);
  assert(result == vk::Result::eSuccess);

  auto cmdAI = vk::CommandBufferAllocateInfo()
                   .setCommandPool(command_pool)
                   .setLevel(vk::CommandBufferLevel::ePrimary)
                   .setCommandBufferCount(1);

  commands = NewArray<vk::CommandBuffer>(swapchain_image_count);
  for (size_t i = 0; i < swapchain_image_count; i++) {
    result =
        renderer->device().allocateCommandBuffers(&cmdAI, &commands[i]);
    assert(result == vk::Result::eSuccess);
  }
}

void Swapchain::CreateSyncObject() {
  vk::Result result;

  const uint32_t frame_count = g_Vars.frame_lag;

  auto semaphoreCreateInfo = vk::SemaphoreCreateInfo();
  auto fenceCI =
      vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);

  device_idle = NewArray<vk::Fence>(frame_count);
  image_acquired = NewArray<vk::Semaphore>(frame_count);
  render_complete = NewArray<vk::Semaphore>(frame_count);

  for (uint32_t i = 0; i < frame_count; i++) {
    result =
        renderer->device().createFence(&fenceCI, nullptr, &device_idle[i]);
    assert(result == vk::Result::eSuccess);

    result = renderer->device().createSemaphore(
        &semaphoreCreateInfo, nullptr, &image_acquired[i]);
    assert(result == vk::Result::eSuccess);

    result = renderer->device().createSemaphore(
        &semaphoreCreateInfo, nullptr, &render_complete[i]);
    assert(result == vk::Result::eSuccess);
  }
  frame_index = 0;
}
}  // namespace impl
}  // namespace VPP
#include "Device.h"

#include <SDL2/SDL_vulkan.h>

#include <iostream>

#include "DrawCmd.h"
#include "VPP_Config.h"

namespace VPP {
namespace impl {

static VkBool32
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      level,
              VkDebugUtilsMessageTypeFlagsEXT             type,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void*                                       pUserData) {
  if (level & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    return VK_TRUE;
  }
  std::cerr << "[vulkan] ";
  switch (level) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    std::cerr << "Warn: ";
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    std::cerr << "Error: ";
    break;
  default:
    break;
  }
  std::cerr << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

static std::vector<const char*> GetWindowExtensions(SDL_Window* window) {
  std::vector<const char*> extensions{};

  uint32_t extensionCount = 0;
  if (SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr) ==
      SDL_TRUE) {
    extensions.resize(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount,
                                     extensions.data());
  }

  return extensions;
}

Device::Device(Window* window, const DeviceOption& options) : window_(window_) {
  CreateInstanceAndSurface(window->window(), options.enable_debug);
  SetGpuAndIndices();
  CreateDeviceAndQueue(options.enable_debug);
  CreateSwapchainObject(swapchain_, vk::ImageUsageFlagBits::eTransferDst);
  CreateSyncObject();
}

Device::~Device() {
  device_.waitIdle();
  if (device_) {
    if (swapchain_) {
      DestroySwapchainObject(swapchain_);
    }
    DestroySwapchainResource();

    for (uint32_t i = 0; i < frame_count_; i++) {
      device_.destroy(fences_[i]);
      device_.destroy(image_acquired_[i]);
      device_.destroy(render_complete_[i]);
    }

    device_.destroy();
  }
  if (instance_) {
    if (surface_) {
      instance_.destroy(surface_);
    }
    instance_.destroy();
  }
}

bool Device::FindMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask,
                            uint32_t& typeIndex) const {
  if (!gpu_) {
    return false;
  }
  auto props = gpu_.getMemoryProperties();
  for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
    if ((memType & 1) == 1) {
      if ((props.memoryTypes[i].propertyFlags & mask) == mask) {
        typeIndex = i;
        return true;
      }
    }
    memType >>= 1;
  }

  return false;
}

void Device::ReCreateSwapchain() {
  device_.waitIdle();

  for (int i = 0; i < FRAME_LAG; i++) {
    device_.waitForFences(1, &fences_[i], VK_TRUE, UINT64_MAX);
  }

  DestroySwapchainResource();
  CreateSwapchainObject(swapchain_, vk::ImageUsageFlagBits::eTransferDst);
}

void Device::set_cmd(const DrawParam& cmd) { cmd_ = &cmd; }

void Device::Draw() {
  device_.waitForFences(1, &fences_[0], VK_TRUE, UINT64_MAX);
  device_.resetFences(1, &fences_[0]);

  auto& curBuf = current_buffer_;

  vk::Result result;
  do {
    result = device_.acquireNextImageKHR(swapchain_.object, UINT64_MAX,
                                         image_acquired_[frame_index_],
                                         vk::Fence(), &curBuf);
    if (result == vk::Result::eErrorOutOfDateKHR) {
      ReCreateSwapchain();
      return;
    } else if (result == vk::Result::eSuboptimalKHR) {
      ReCreateSwapchain();
      return;
    } else {
    }
  } while (result != vk::Result::eSuccess);

  cmd_->Call(commands_[0], framebuffers_, render_pass_);

  vk::PipelineStageFlags pipeStageFlags =
      vk::PipelineStageFlagBits::eColorAttachmentOutput;
  auto const submitInfo =
      vk::SubmitInfo()
          .setPWaitDstStageMask(&pipeStageFlags)
          .setWaitSemaphoreCount(1)
          .setPWaitSemaphores(&image_acquired_[frame_index_])
          .setCommandBufferCount(1)
          .setPCommandBuffers(&commands_[0])
          .setSignalSemaphoreCount(1)
          .setPSignalSemaphores(&render_complete_[frame_index_]);

  result = graphics_.queue.submit(1, &submitInfo, fences_[0]);
  assert(result == vk::Result::eSuccess);

  cmd_->Present(swapchain_.images[curBuf]);

  auto const presentInfo =
      vk::PresentInfoKHR()
          .setWaitSemaphoreCount(1)
          .setPWaitSemaphores(&render_complete_[frame_index_])
          .setSwapchainCount(1)
          .setPSwapchains(&swapchain_.object)
          .setPImageIndices(&curBuf);

  result = present_.queue.presentKHR(&presentInfo);
  frame_index_ += 1;
  frame_index_ %= FRAME_LAG;
  if (result == vk::Result::eErrorOutOfDateKHR) {
    ReCreateSwapchain();
    return;
  } else if (result == vk::Result::eSuboptimalKHR) {
    ReCreateSwapchain();
    return;
  } else {
  }
}

void Device::EndDraw() {
  device_.waitIdle();

  for (int i = 0; i < FRAME_LAG; i++) {
    device_.waitForFences(1, &fences_[i], VK_TRUE, UINT64_MAX);
    device_.destroy(fences_[i]);
    device_.destroy(image_acquired_[i]);
    device_.destroy(render_complete_[i]);
  }
  frame_count_ = 0;
  fences_.reset();
  image_acquired_.reset();
  render_complete_.reset();
}

bool Device::CreateSwapchainObject(SwapchainObject&    swapchain,
                                   vk::ImageUsageFlags usages) {
  vk::Result result = vk::Result::eSuccess;

  auto caps          = gpu_.getSurfaceCapabilitiesKHR(surface_);
  auto presentModes  = gpu_.getSurfacePresentModesKHR(surface_);
  auto surfaceFormat = gpu_.getSurfaceFormatsKHR(surface_);

  auto oldSwapchain = swapchain.object;

  auto presentMode = vk::PresentModeKHR::eFifo;
  auto surfFormat  = surfaceFormat[0];
  auto curExtent   = caps.currentExtent;
  if (curExtent.width == 0 || curExtent.height == 0) {
    SDL_Vulkan_GetDrawableSize(window_->window(), (int*)&curExtent.width,
                               (int*)&curExtent.height);
  }

  auto swapCI =
      vk::SwapchainCreateInfoKHR()
          .setImageArrayLayers(1)
          .setClipped(true)
          .setSurface(surface_)
          .setMinImageCount(caps.minImageCount)
          .setImageFormat(surfFormat.format)
          .setImageColorSpace(surfFormat.colorSpace)
          .setImageExtent(curExtent)
          .setImageUsage(usages | vk::ImageUsageFlagBits::eColorAttachment)
          .setPreTransform(caps.currentTransform)
          .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
          .setPresentMode(presentMode);

  std::vector<uint32_t> indices = {graphics_.index};
  if (graphics_.index == present_.index) {
    swapCI.setImageSharingMode(vk::SharingMode::eExclusive);
  } else {
    swapCI.setImageSharingMode(vk::SharingMode::eConcurrent);
    indices.push_back(present_.index);
  }
  swapCI.setQueueFamilyIndices(indices);

  swapCI.setOldSwapchain(oldSwapchain);

  if (device_.createSwapchainKHR(&swapCI, nullptr, &swapchain.object) ==
      vk::Result::eSuccess) {
    return false;
  }

  if (oldSwapchain) {
    device_.destroy(oldSwapchain);
  }

  swapchain.extent       = curExtent;
  swapchain.present_mode = presentMode;
  swapchain.format       = surfFormat.format;
  swapchain.usage        = usages;

  swapchain.images = device_.getSwapchainImagesKHR(swapchain.object);
  if (swapchain.ImageCount() == 0) {
    return false;
  }

  CreateSwapchainImageViews(surfaceFormat[0].format);
  CreateDepthbuffer(caps.currentExtent);
  CreateRenderPass(vk::Format::eB8G8R8A8Unorm);
  CreateFramebuffers(caps.currentExtent);
  CreateCommandBuffers();
}

void Device::DestroySwapchainObject(SwapchainObject& swapchain) {
  device_.destroy(swapchain.object);
  swapchain.object = VK_NULL_HANDLE;
}

void Device::DestroySwapchainResource() {
  for (uint32_t i = 0; i < swapchain_.ImageCount(); ++i) {
    if (swapchain_imageviews_[i]) {
      device_.destroy(swapchain_imageviews_[i]);
    }
  }
  if (framebuffers_) {
    device_.destroy(framebuffers_);
  }

  if (command_pool_) {
    device_.destroy(command_pool_);
  }

  if (render_pass_) {
    device_.destroy(render_pass_);
  }

  if (depth_image_) {
    device_.destroy(depth_image_);
  }

  if (depth_imageview_) {
    device_.destroy(depth_imageview_);
  }

  if (depth_memory_) {
    device_.free(depth_memory_);
  }

  if (color_image_) {
    device_.destroy(color_image_);
  }

  if (color_imageview_) {
    device_.destroy(color_imageview_);
  }

  if (color_memory_) {
    device_.free(color_memory_);
  }
}

void Device::CreateSwapchainImageViews(vk::Format format) {
  vk::Result result = vk::Result::eSuccess;

  auto resRange = vk::ImageSubresourceRange()
                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                      .setLayerCount(1)
                      .setBaseArrayLayer(0)
                      .setLevelCount(1)
                      .setBaseMipLevel(0);

  swapchain_imageviews_ =
      std::make_unique<vk::ImageView[]>(swapchain_.ImageCount());

  for (uint32_t i = 0; i < swapchain_.ImageCount(); i++) {
    vk::ImageViewCreateInfo imageViewCI =
        vk::ImageViewCreateInfo()
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(format)
            .setPNext(nullptr)
            .setSubresourceRange(resRange);
    imageViewCI.setImage(swapchain_.images[i]);
    result = device_.createImageView(&imageViewCI, nullptr,
                                     &swapchain_imageviews_[i]);
    assert(result == vk::Result::eSuccess);
  }
}

void Device::CreateDepthbuffer(vk::Extent2D extent) {
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

  std::vector<uint32_t> indices = {graphics_.index};
  if (graphics_.index == present_.index) {
    imageCI.setSharingMode(vk::SharingMode::eExclusive);
  } else {
    imageCI.setSharingMode(vk::SharingMode::eConcurrent);
    indices.push_back(present_.index);
  }
  imageCI.setQueueFamilyIndices(indices);

  {
    imageCI.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
    imageCI.setFormat(vk::Format::eD16Unorm);
    result = device_.createImage(&imageCI, nullptr, &depth_image_);
    assert(result == vk::Result::eSuccess);
  }

  {
    imageCI.setUsage(vk::ImageUsageFlagBits::eColorAttachment |
                     vk::ImageUsageFlagBits::eTransferSrc);
    imageCI.setFormat(vk::Format::eB8G8R8A8Unorm);
    result = device_.createImage(&imageCI, nullptr, &color_image_);
    assert(result == vk::Result::eSuccess);
  }

  {
    vk::MemoryAllocateInfo memoryAI;
    vk::MemoryRequirements memReq;
    device_.getImageMemoryRequirements(depth_image_, &memReq);
    memoryAI.setAllocationSize(memReq.size);
    auto pass = FindMemoryType(memReq.memoryTypeBits,
                               vk::MemoryPropertyFlagBits::eDeviceLocal,
                               memoryAI.memoryTypeIndex);
    assert(pass);
    result = device_.allocateMemory(&memoryAI, nullptr, &depth_memory_);
    assert(result == vk::Result::eSuccess);
    device_.bindImageMemory(depth_image_, depth_memory_, 0);
  }

  {
    vk::MemoryAllocateInfo memoryAI;
    vk::MemoryRequirements memReq;
    device_.getImageMemoryRequirements(color_image_, &memReq);
    memoryAI.setAllocationSize(memReq.size);
    auto pass = FindMemoryType(memReq.memoryTypeBits,
                               vk::MemoryPropertyFlagBits::eDeviceLocal,
                               memoryAI.memoryTypeIndex);
    assert(pass);
    result = device_.allocateMemory(&memoryAI, nullptr, &color_memory_);
    assert(result == vk::Result::eSuccess);
    device_.bindImageMemory(color_image_, color_memory_, 0);
  }

  auto imageViewCI =
      vk::ImageViewCreateInfo().setViewType(vk::ImageViewType::e2D);
  imageViewCI.subresourceRange.baseMipLevel   = 0;
  imageViewCI.subresourceRange.baseArrayLayer = 0;
  imageViewCI.subresourceRange.levelCount     = 1;
  imageViewCI.subresourceRange.layerCount     = 1;

  {
    imageViewCI.setImage(depth_image_);
    imageViewCI.setFormat(vk::Format::eD16Unorm);
    imageViewCI.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    result = device_.createImageView(&imageViewCI, nullptr, &depth_imageview_);
    assert(result == vk::Result::eSuccess);
  }

  {
    imageViewCI.setImage(color_image_);
    imageViewCI.setFormat(vk::Format::eB8G8R8A8Unorm);
    imageViewCI.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    result = device_.createImageView(&imageViewCI, nullptr, &color_imageview_);
    assert(result == vk::Result::eSuccess);
  }
}

void Device::CreateRenderPass(vk::Format format) {
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
          .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal),
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

  result = device_.createRenderPass(&renderPassCI, nullptr, &render_pass_);
  assert(result == vk::Result::eSuccess);
}

void Device::CreateFramebuffers(vk::Extent2D extent) {
  vk::ImageView attachments[2];
  attachments[1] = depth_imageview_;

  auto frameBufferCI = vk::FramebufferCreateInfo()
                           .setRenderPass(render_pass_)
                           .setAttachmentCount(2)
                           .setPAttachments(attachments)
                           .setWidth(extent.width)
                           .setHeight(extent.height)
                           .setLayers(1);

  attachments[0] = color_imageview_;
  auto result =
      device_.createFramebuffer(&frameBufferCI, nullptr, &framebuffers_);
  assert(result == vk::Result::eSuccess);
}

void Device::CreateCommandBuffers() {
  vk::Result result;

  auto cmdPoolCI =
      vk::CommandPoolCreateInfo()
          .setQueueFamilyIndex(graphics_.index)
          .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
  result = device_.createCommandPool(&cmdPoolCI, nullptr, &command_pool_);
  assert(result == vk::Result::eSuccess);

  auto cmdAI = vk::CommandBufferAllocateInfo()
                   .setCommandPool(command_pool_)
                   .setLevel(vk::CommandBufferLevel::ePrimary)
                   .setCommandBufferCount(1);

  commands_ = std::make_unique<vk::CommandBuffer[]>(swapchain_.ImageCount());
  for (size_t i = 0; i < swapchain_.ImageCount(); i++) {
    result = device_.allocateCommandBuffers(&cmdAI, &commands_[i]);
    assert(result == vk::Result::eSuccess);
  }
}

void Device::CreateInstanceAndSurface(SDL_Window* window, bool enableDbg) {
  vk::Result result = vk::Result::eSuccess;

  auto appCI = vk::ApplicationInfo()
                   .setPNext(nullptr)
                   .setPApplicationName("Vulkan Engine")
                   .setApplicationVersion(0)
                   .setApiVersion(VK_API_VERSION_1_1)
                   .setPEngineName("None")
                   .setEngineVersion(0);

  std::vector<const char*> enabledLayers;
  auto                     enabledExtensions = GetWindowExtensions(window);
  if (enableDbg) {
    enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
  }

  auto instCI = vk::InstanceCreateInfo()
                    .setPEnabledLayerNames(enabledLayers)
                    .setPEnabledExtensionNames(enabledExtensions)
                    .setPApplicationInfo(&appCI);

  result = vk::createInstance(&instCI, nullptr, &instance_);
  assert(result == vk::Result::eSuccess);

  VkSurfaceKHR cSurf;
  SDL_Vulkan_CreateSurface(window, instance_, &cSurf);
  surface_ = cSurf;
  assert(surface_);
}

void Device::SetGpuAndIndices() {
  auto availableGPUs = instance_.enumeratePhysicalDevices();
  bool found         = false;

  for (const auto& curGpu : availableGPUs) {
    auto properties = curGpu.getProperties();
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      gpu_  = curGpu;
      found = true;
      break;
    }
  }
  assert(found);

  auto     queueProperties = gpu_.getQueueFamilyProperties();
  uint32_t indexCount      = (uint32_t)queueProperties.size();

  for (uint32_t i = 0; i < indexCount; ++i) {
    if (queueProperties[i].queueFlags & vk::QueueFlagBits::eGraphics &&
        graphics_.index == UINT32_MAX) {
      graphics_.index = i;
    }

    if (queueProperties[i].queueFlags & vk::QueueFlagBits::eTransfer &&
        transfer_.index == UINT32_MAX) {
      transfer_.index = i;
    }

    if (gpu_.getSurfaceSupportKHR(i, surface_) == VK_TRUE &&
        present_.index == UINT32_MAX) {
      present_.index = i;
    }

    if (graphics_.index != UINT32_MAX && transfer_.index != UINT32_MAX &&
        present_.index != UINT32_MAX) {
      break;
    }
  }
  assert(graphics_.index != UINT32_MAX && transfer_.index != UINT32_MAX &&
         present_.index != UINT32_MAX);

  property_ = gpu_.getProperties();
}

void Device::CreateDeviceAndQueue(bool enableDbg) {
  vk::Result result = vk::Result::eSuccess;

  struct QueueIndexAlloc {
    uint32_t   family_index;
    uint32_t   queue_index;
    float      priority = 1.f;
    vk::Queue* queue;
  };
  std::vector<QueueIndexAlloc> allocs;
  auto PushAlloc = [&allocs](QueueReference& queueRef, float priority) {
    uint32_t counter = 0;
    for (const auto& e : allocs) {
      if (e.family_index == queueRef.index) {
        ++counter;
      }
    }
    allocs.emplace_back(
        QueueIndexAlloc{queueRef.index, counter, priority, &queueRef.queue});
  };
  PushAlloc(graphics_, 1.f);
  PushAlloc(transfer_, 1.f);
  PushAlloc(present_, 1.f);

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
  float                                  queuePriority = 1.0f;
  for (const auto& al : allocs) {
    auto queueCreateInfo = vk::DeviceQueueCreateInfo()
                               .setQueueFamilyIndex(al.family_index)
                               .setQueueCount(1)
                               .setPQueuePriorities(&al.priority);
    queueCreateInfos.push_back(queueCreateInfo);
  }

  std::vector<const char*> enabledExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  std::vector<const char*> enabledLayers;
  if (enableDbg) {
    enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
  }

  vk::DeviceCreateInfo deviceCI =
      vk::DeviceCreateInfo()
          .setQueueCreateInfoCount(1)
          .setQueueCreateInfos(queueCreateInfos)
          .setPEnabledExtensionNames(enabledExtensions)
          .setPEnabledLayerNames(enabledLayers)
          .setPEnabledFeatures(nullptr);

  result = gpu_.createDevice(&deviceCI, nullptr, &device_);
  assert(result == vk::Result::eSuccess);

  for (auto& al : allocs) {
    device_.getQueue(al.family_index, al.queue_index, al.queue);
  }
}

void Device::CreateSyncObject() {
  vk::Result result;

  frame_count_ = FRAME_LAG;

  auto semaphoreCreateInfo = vk::SemaphoreCreateInfo();
  auto fenceCI =
      vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);

  fences_          = std::make_unique<vk::Fence[]>(frame_count_);
  image_acquired_  = std::make_unique<vk::Semaphore[]>(frame_count_);
  render_complete_ = std::make_unique<vk::Semaphore[]>(frame_count_);

  for (uint32_t i = 0; i < frame_count_; i++) {
    result = device_.createFence(&fenceCI, nullptr, &fences_[i]);
    assert(result == vk::Result::eSuccess);

    result = device_.createSemaphore(&semaphoreCreateInfo, nullptr,
                                     &image_acquired_[i]);
    assert(result == vk::Result::eSuccess);

    result = device_.createSemaphore(&semaphoreCreateInfo, nullptr,
                                     &render_complete_[i]);
    assert(result == vk::Result::eSuccess);
  }
  frame_index_ = 0;
}

DeviceResource::DeviceResource(Device* device) : parent_(device) {}

DeviceResource::~DeviceResource() {}

vk::DeviceMemory
DeviceResource::CreateMemory(const vk::MemoryRequirements& req,
                             vk::MemoryPropertyFlags       flags) const {
  vk::MemoryAllocateInfo memoryAI = vk::MemoryAllocateInfo();
  memoryAI.setAllocationSize(req.size);
  memoryAI.setMemoryTypeIndex(0);
  if (!parent_->FindMemoryType(req.memoryTypeBits, flags,
                               memoryAI.memoryTypeIndex)) {
    return VK_NULL_HANDLE;
  }
  return device().allocateMemory(memoryAI);
}

vk::Buffer DeviceResource::CreateBuffer(vk::BufferUsageFlags flags,
                                        size_t               size) const {
  auto bufferCI = vk::BufferCreateInfo()
                      .setUsage(flags)
                      .setQueueFamilyIndexCount(0)
                      .setPQueueFamilyIndices(nullptr)
                      .setSharingMode(vk::SharingMode::eExclusive)
                      .setSize(size);
  return device().createBuffer(bufferCI);
}

bool DeviceResource::CopyBuffer2Buffer(const vk::Buffer& srcBuffer,
                                       const vk::Buffer& dstBuffer,
                                       size_t            size) const {
  auto cmd = BeginOnceCmd();
  if (!cmd) {
    return false;
  }
  auto copyRegion =
      vk::BufferCopy().setDstOffset(0).setSrcOffset(0).setSize(size);
  cmd.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
  EndOnceCmd(cmd);
  return true;
}

bool DeviceResource::CopyBuffer2Image(const vk::Buffer& srcBuffer,
                                      const vk::Image& dstImage, uint32_t width,
                                      uint32_t height, uint32_t channel) const {
  auto cmd = BeginOnceCmd();
  if (!cmd) {
    return false;
  }
  SetImageForTransfer(cmd, dstImage);
  auto region = vk::BufferImageCopy()
                    .setBufferOffset(0)
                    .setBufferRowLength(width)
                    .setBufferImageHeight(height)
                    .setImageSubresource(vk::ImageSubresourceLayers{
                        vk::ImageAspectFlagBits::eColor, 0, 0, 1})
                    .setImageOffset(vk::Offset3D{0, 0, 0})
                    .setImageExtent(vk::Extent3D{width, height, 1});
  cmd.copyBufferToImage(srcBuffer, dstImage,
                        vk::ImageLayout::eTransferDstOptimal, 1, &region);
  SetImageForShader(cmd, dstImage);
  EndOnceCmd(cmd);
  return true;
}

bool DeviceResource::CopyPresent(const vk::Image& to) const {
  auto cmd = BeginOnceCmd();

  if (!cmd) {
    return false;
  }

  {
    auto barrier = vk::ImageMemoryBarrier()
                       .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
                       .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
                       .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setImage(parent_->color_image_)
                       .setSubresourceRange(vk::ImageSubresourceRange{
                           vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
                       .setSrcAccessMask((vk::AccessFlags)0)
                       .setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe,
                        vk::PipelineStageFlagBits::eTransfer,
                        (vk::DependencyFlagBits)0, 0, nullptr, 0, nullptr, 1,
                        &barrier);
  }
  {
    auto barrier = vk::ImageMemoryBarrier()
                       .setOldLayout(vk::ImageLayout::eUndefined)
                       .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                       .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setImage(to)
                       .setSubresourceRange(vk::ImageSubresourceRange{
                           vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
                       .setSrcAccessMask((vk::AccessFlags)0)
                       .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                        vk::PipelineStageFlagBits::eTransfer,
                        (vk::DependencyFlagBits)0, 0, nullptr, 0, nullptr, 1,
                        &barrier);
  }

  const auto& extent = parent_->swapchain_.extent;
  auto        region = vk::ImageCopy().setExtent(vk::Extent3D{extent, 1});
  region.srcSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
  region.srcSubresource.mipLevel       = 0;
  region.srcSubresource.layerCount     = 1;
  region.srcSubresource.baseArrayLayer = 0;
  region.dstSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
  region.dstSubresource.mipLevel       = 0;
  region.dstSubresource.layerCount     = 1;
  region.dstSubresource.baseArrayLayer = 0;
  cmd.copyImage(parent_->color_image_, vk::ImageLayout::eTransferSrcOptimal, to,
                vk::ImageLayout::eTransferDstOptimal, 1, &region);

  {
    auto barrier = vk::ImageMemoryBarrier()
                       .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                       .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
                       .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setImage(to)
                       .setSubresourceRange(vk::ImageSubresourceRange{
                           vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
                       .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                       .setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eBottomOfPipe,
                        (vk::DependencyFlagBits)0, 0, nullptr, 0, nullptr, 1,
                        &barrier);
  }

  EndOnceCmd(cmd);
  return true;
}

vk::CommandBuffer DeviceResource::BeginOnceCmd() const {
  auto cmdAI = vk::CommandBufferAllocateInfo()
                   .setCommandPool(parent_->command_pool_)
                   .setLevel(vk::CommandBufferLevel::ePrimary)
                   .setCommandBufferCount(1);

  vk::CommandBuffer cmd    = VK_NULL_HANDLE;
  auto              result = device().allocateCommandBuffers(&cmdAI, &cmd);
  if (result != vk::Result::eSuccess) {
    return cmd;
  }

  auto beginInfo = vk::CommandBufferBeginInfo().setFlags(
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  cmd.begin(beginInfo);
  return cmd;
}

void DeviceResource::EndOnceCmd(vk::CommandBuffer& cmd) const {
  cmd.end();

  parent_->graphics_.queue.waitIdle();
  auto submitInfo =
      vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&cmd);
  parent_->graphics_.queue.submit(1, &submitInfo, VK_NULL_HANDLE);
  parent_->graphics_.queue.waitIdle();

  device().free(parent_->command_pool_, 1, &cmd);
  cmd = VK_NULL_HANDLE;
}

void DeviceResource::SetImageForTransfer(const vk::CommandBuffer& cmd,
                                         const vk::Image&         image) const {
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
  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                      vk::PipelineStageFlagBits::eTransfer,
                      (vk::DependencyFlagBits)0, 0, nullptr, 0, nullptr, 1,
                      &barrier);
}

void DeviceResource::SetImageForShader(const vk::CommandBuffer& cmd,
                                       const vk::Image&         image) const {
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
  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                      vk::PipelineStageFlagBits::eFragmentShader |
                          vk::PipelineStageFlagBits::eVertexShader,
                      (vk::DependencyFlagBits)0, 0, nullptr, 0, nullptr, 1,
                      &barrier);
}

StageBuffer::StageBuffer(const void* data, size_t size)
    : DeviceResource(), size_(size) {
  buffer_ = CreateBuffer(vk::BufferUsageFlagBits::eTransferSrc, size_);
  if (!buffer_) {
    return;
  }
  memory_ = CreateMemory(device().getBufferMemoryRequirements(buffer_),
                         vk::MemoryPropertyFlagBits::eHostVisible |
                             vk::MemoryPropertyFlagBits::eHostCoherent);
  if (!memory_) {
    return;
  }
  device().bindBufferMemory(buffer_, memory_, 0);
  auto* mapData = device().mapMemory(memory_, 0, size_);
  memcpy(mapData, data, size_);
  device().unmapMemory(memory_);
}

StageBuffer::~StageBuffer() {
  if (buffer_) {
    device().destroy(buffer_);
  }
  if (memory_) {
    device().free(memory_);
  }
}

bool StageBuffer::CopyTo(const vk::Buffer& dstBuffer) {
  return CopyBuffer2Buffer(buffer_, dstBuffer, size_);
}

bool StageBuffer::CopyTo(const vk::Image& dstImage, uint32_t width,
                         uint32_t height, uint32_t channel) {
  return CopyBuffer2Image(buffer_, dstImage, width, height, channel);
}

} // namespace impl
} // namespace VPP
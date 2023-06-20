#include "Device.h"

#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <set>

#include "DrawCmd.h"
#include "VPP_Config.h"

namespace VPP {
namespace impl {
static std::vector<const char*> kEnabledLayers{"VK_LAYER_KHRONOS_validation"};

static VkBool32
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT level,
              VkDebugUtilsMessageTypeFlagsEXT type,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData) {
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

Device::Device(Window* window) {
  CreateInstance(window->window_);
  CreateSurface(window->window_);
  SetGpuAndIndices();
  CreateDevice();
  GetQueues();
  CreateSwapchainResource(VK_NULL_HANDLE);
  CreateSyncObject();
}

Device::~Device() {
  device_.waitIdle();
  if (device_) {
    if (swapchain_) {
      device_.destroy(swapchain_);
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
  vk::SwapchainKHR oldSwapchain = swapchain_;
  CreateSwapchainResource(oldSwapchain);
  for (uint32_t i = 0; i < swapchain_image_count_; i++) {
    cmd_->Call(commands_[i], framebuffers_[i], render_pass_);
  }
  device_.destroy(oldSwapchain);
}

void Device::set_cmd(const DrawCmd& cmd) {
  cmd_ = &cmd;
  for (uint32_t i = 0; i < swapchain_image_count_; i++) {
    cmd_->Call(commands_[i], framebuffers_[i], render_pass_);
  }
}

void Device::Draw() {
  device_.waitForFences(1, &fences_[frame_index_], VK_TRUE, UINT64_MAX);
  device_.resetFences(1, &fences_[frame_index_]);

  auto& curBuf = current_buffer_;

  vk::Result result;
  do {
    result = device_.acquireNextImageKHR(swapchain_, UINT64_MAX,
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

  vk::PipelineStageFlags pipeStageFlags =
      vk::PipelineStageFlagBits::eColorAttachmentOutput;
  auto const submitInfo =
      vk::SubmitInfo()
          .setPWaitDstStageMask(&pipeStageFlags)
          .setWaitSemaphoreCount(1)
          .setPWaitSemaphores(&image_acquired_[frame_index_])
          .setCommandBufferCount(1)
          .setPCommandBuffers(&commands_[curBuf])
          .setSignalSemaphoreCount(1)
          .setPSignalSemaphores(&render_complete_[frame_index_]);

  result = graphics_queue_.submit(1, &submitInfo, fences_[frame_index_]);
  assert(result == vk::Result::eSuccess);

  auto const presentInfo =
      vk::PresentInfoKHR()
          .setWaitSemaphoreCount(1)
          .setPWaitSemaphores(&render_complete_[frame_index_])
          .setSwapchainCount(1)
          .setPSwapchains(&swapchain_)
          .setPImageIndices(&curBuf);

  result = present_queue_.presentKHR(&presentInfo);
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

void Device::CreateSwapchainResource(vk::SwapchainKHR oldSwapchain) {
  vk::Result result = vk::Result::eSuccess;

  auto caps = gpu_.getSurfaceCapabilitiesKHR(surface_);
  auto presentModes = gpu_.getSurfacePresentModesKHR(surface_);
  auto surfaceFormat = gpu_.getSurfaceFormatsKHR(surface_);

  auto swapCI = vk::SwapchainCreateInfoKHR()
                    .setImageArrayLayers(1)
                    .setClipped(true)
                    .setSurface(surface_)
                    .setMinImageCount(caps.minImageCount)
                    .setImageFormat(surfaceFormat[0].format)
                    .setImageColorSpace(surfaceFormat[0].colorSpace)
                    .setImageExtent(caps.currentExtent)
                    .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                    .setPreTransform(caps.currentTransform)
                    .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                    .setPresentMode(presentModes[0]);

  std::vector<uint32_t> indices = {graphics_index_};
  if (graphics_index_ == present_index_) {
    swapCI.setImageSharingMode(vk::SharingMode::eExclusive);
  } else {
    swapCI.setImageSharingMode(vk::SharingMode::eConcurrent);
    indices.push_back(present_index_);
  }
  swapCI.setQueueFamilyIndices(indices);

  swapCI.setOldSwapchain(oldSwapchain);

  result = device_.createSwapchainKHR(&swapCI, nullptr, &swapchain_);
  assert(result == vk::Result::eSuccess);

  extent_ = caps.currentExtent;

  GetSwapchainImages();
  CreateSwapchainImageViews(surfaceFormat[0].format);
  CreateDepthbuffer(caps.currentExtent);
  CreateRenderPass(surfaceFormat[0].format);
  CreateFramebuffers(caps.currentExtent);
  CreateCommandBuffers();
}

void Device::DestroySwapchainResource() {
  for (uint32_t i = 0; i < swapchain_image_count_; ++i) {
    if (framebuffers_[i]) {
      device_.destroy(framebuffers_[i]);
    }

    if (swapchain_imageviews_[i]) {
      device_.destroy(swapchain_imageviews_[i]);
    }
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
}

void Device::GetSwapchainImages() {
  vk::Result result = vk::Result::eSuccess;

  result = device_.getSwapchainImagesKHR(swapchain_, &swapchain_image_count_,
                                         nullptr);
  assert(result == vk::Result::eSuccess);
  swapchain_images_ = NewArray<vk::Image>(swapchain_image_count_);
  result = device_.getSwapchainImagesKHR(swapchain_, &swapchain_image_count_,
                                         swapchain_images_.get());
  assert(result == vk::Result::eSuccess);
}

void Device::CreateSwapchainImageViews(vk::Format format) {
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

  std::vector<uint32_t> indices = {graphics_index_};
  if (graphics_index_ == present_index_) {
    imageCI.setSharingMode(vk::SharingMode::eExclusive);
  } else {
    imageCI.setSharingMode(vk::SharingMode::eConcurrent);
    indices.push_back(present_index_);
  }
  imageCI.setQueueFamilyIndices(indices);
  result = device_.createImage(&imageCI, nullptr, &depth_image_);
  assert(result == vk::Result::eSuccess);

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
  auto imageViewCI = vk::ImageViewCreateInfo()
                         .setImage(depth_image_)
                         .setViewType(vk::ImageViewType::e2D)
                         .setFormat(vk::Format::eD16Unorm)
                         .setSubresourceRange(vk::ImageSubresourceRange(
                             vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1))
                         .setPNext(nullptr);
  result = device_.createImageView(&imageViewCI, nullptr, &depth_imageview_);
  assert(result == vk::Result::eSuccess);
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

  result = device_.createRenderPass(&renderPassCI, nullptr, &render_pass_);
  assert(result == vk::Result::eSuccess);
}

void Device::CreateFramebuffers(vk::Extent2D extent) {
  framebuffers_ = NewArray<vk::Framebuffer>(swapchain_image_count_);

  vk::ImageView attachments[2];
  attachments[1] = depth_imageview_;

  auto frameBufferCI = vk::FramebufferCreateInfo()
                           .setRenderPass(render_pass_)
                           .setAttachmentCount(2)
                           .setPAttachments(attachments)
                           .setWidth(extent.width)
                           .setHeight(extent.height)
                           .setLayers(1);

  for (uint32_t i = 0; i < swapchain_image_count_; i++) {
    attachments[0] = swapchain_imageviews_[i];
    auto result =
        device_.createFramebuffer(&frameBufferCI, nullptr, &framebuffers_[i]);
    assert(result == vk::Result::eSuccess);
  }
}

void Device::CreateCommandBuffers() {
  vk::Result result;

  auto cmdPoolCI =
      vk::CommandPoolCreateInfo().setQueueFamilyIndex(graphics_index_);
  result = device_.createCommandPool(&cmdPoolCI, nullptr, &command_pool_);
  assert(result == vk::Result::eSuccess);

  auto cmdAI = vk::CommandBufferAllocateInfo()
                   .setCommandPool(command_pool_)
                   .setLevel(vk::CommandBufferLevel::ePrimary)
                   .setCommandBufferCount(1);

  commands_ = NewArray<vk::CommandBuffer>(swapchain_image_count_);
  for (size_t i = 0; i < swapchain_image_count_; i++) {
    result = device_.allocateCommandBuffers(&cmdAI, &commands_[i]);
    assert(result == vk::Result::eSuccess);
  }
}

void Device::CreateInstance(SDL_Window* window) {
  vk::Result result = vk::Result::eSuccess;

  auto appCI = vk::ApplicationInfo()
                   .setPNext(nullptr)
                   .setPApplicationName("Vulkan Engine")
                   .setApplicationVersion(0)
                   .setApiVersion(VK_API_VERSION_1_1)
                   .setPEngineName("None")
                   .setEngineVersion(0);

  auto extensions = GetWindowExtensions(window);
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  using MsgSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
  using MsgType = vk::DebugUtilsMessageTypeFlagBitsEXT;
  auto debugCI =
      vk::DebugUtilsMessengerCreateInfoEXT()
          .setPNext(nullptr)
          .setMessageSeverity(MsgSeverity::eInfo | MsgSeverity::eWarning |
                              MsgSeverity::eError)
          .setMessageType(MsgType::eGeneral | MsgType::eValidation |
                          MsgType::ePerformance)
          .setPfnUserCallback(DebugCallback);

  auto instCI = vk::InstanceCreateInfo()
                    .setPEnabledLayerNames(kEnabledLayers)
                    .setPEnabledExtensionNames(extensions)
                    .setPApplicationInfo(&appCI)
                    .setPNext(&debugCI);

  result = vk::createInstance(&instCI, nullptr, &instance_);
  assert(result == vk::Result::eSuccess);
}

void Device::CreateSurface(SDL_Window* window) {
  VkSurfaceKHR cSurf;
  SDL_Vulkan_CreateSurface(window, instance_, &cSurf);
  surface_ = cSurf;
  assert(surface_);
}

void Device::SetGpuAndIndices() {
  auto availableGPUs = instance_.enumeratePhysicalDevices();
  bool found = false;

  for (const auto& curGpu : availableGPUs) {
    auto properties = curGpu.getProperties();
    auto features = curGpu.getFeatures();
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      gpu_ = curGpu;
      found = true;
      break;
    }
  }
  assert(found);

  auto queueProperties = gpu_.getQueueFamilyProperties();
  uint32_t indexCount = (uint32_t)queueProperties.size();

  std::vector<vk::Bool32> supportsPresent(indexCount);
  for (uint32_t i = 0; i < indexCount; i++) {
    gpu_.getSurfaceSupportKHR(i, surface_, &supportsPresent[i]);
  }

  for (uint32_t i = 0; i < indexCount; ++i) {
    if (queueProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
      graphics_index_ = i;
    }

    if (supportsPresent[i] == VK_TRUE) {
      present_index_ = i;
    }

    if (graphics_index_ != UINT32_MAX && present_index_ != UINT32_MAX) {
      break;
    }
  }
  assert(graphics_index_ != UINT32_MAX && present_index_ != UINT32_MAX);

  property_ = gpu_.getProperties();
}

void Device::CreateDevice() {
  vk::Result result = vk::Result::eSuccess;

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
  std::set<uint32_t> queueFamilies = {graphics_index_, present_index_};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : queueFamilies) {
    auto queueCreateInfo = vk::DeviceQueueCreateInfo()
                               .setQueueFamilyIndex(queueFamily)
                               .setQueueCount(1)
                               .setPQueuePriorities(&queuePriority);
    queueCreateInfos.push_back(queueCreateInfo);
  }

  std::vector<const char*> enabledExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  vk::DeviceCreateInfo deviceCI =
      vk::DeviceCreateInfo()
          .setQueueCreateInfoCount(1)
          .setQueueCreateInfos(queueCreateInfos)
          .setPEnabledExtensionNames(enabledExtensions)
          .setPEnabledLayerNames(kEnabledLayers)
          .setPEnabledFeatures(nullptr);

  result = gpu_.createDevice(&deviceCI, nullptr, &device_);
  assert(result == vk::Result::eSuccess);
}

void Device::GetQueues() {
  graphics_queue_ = device_.getQueue(graphics_index_, 0);
  present_queue_ = device_.getQueue(present_index_, 0);
}

void Device::CreateSyncObject() {
  vk::Result result;

  frame_count_ = FRAME_LAG;

  auto semaphoreCreateInfo = vk::SemaphoreCreateInfo();
  auto fenceCI =
      vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);

  fences_ = NewArray<vk::Fence>(frame_count_);
  image_acquired_ = NewArray<vk::Semaphore>(frame_count_);
  render_complete_ = NewArray<vk::Semaphore>(frame_count_);

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

vk::DeviceMemory
DeviceResource::CreateMemory(const vk::MemoryRequirements& req,
                             vk::MemoryPropertyFlags flags) const {
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
                                        size_t size) const {
  auto bufferCI = vk::BufferCreateInfo()
                      .setUsage(flags)
                      .setQueueFamilyIndexCount(0)
                      .setPQueueFamilyIndices(nullptr)
                      .setSharingMode(vk::SharingMode::eExclusive)
                      .setSize(size);
  return device().createBuffer(bufferCI);
}

bool DeviceResource::CopyBuffer(const vk::Buffer& srcBuffer,
                                const vk::Buffer& dstBuffer, size_t size) {
  auto cmdAI = vk::CommandBufferAllocateInfo()
                   .setCommandPool(parent_->command_pool_)
                   .setLevel(vk::CommandBufferLevel::ePrimary)
                   .setCommandBufferCount(1);

  vk::CommandBuffer cmd = VK_NULL_HANDLE;
  auto result = device().allocateCommandBuffers(&cmdAI, &cmd);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  auto beginInfo = vk::CommandBufferBeginInfo().setFlags(
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  cmd.begin(beginInfo);

  auto copyRegion =
      vk::BufferCopy().setDstOffset(0).setSrcOffset(0).setSize(size);
  cmd.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

  cmd.end();

  parent_->graphics_queue_.waitIdle();
  auto submitInfo =
      vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&cmd);
  parent_->graphics_queue_.submit(1, &submitInfo, VK_NULL_HANDLE);
  parent_->graphics_queue_.waitIdle();

  device().free(parent_->command_pool_, 1, &cmd);
  return true;
}

StageBuffer::StageBuffer(void* data, size_t size)
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

bool StageBuffer::CopyTo(vk::Buffer dstBuffer) {
  return CopyBuffer(buffer_, dstBuffer, size_);
}

extern Device* g_Device;
Device* GetDevice() { return g_Device; }

DeviceResource::DeviceResource() { parent_ = g_Device; }

DeviceResource::~DeviceResource() {}

} // namespace impl
} // namespace VPP
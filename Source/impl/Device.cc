#include "Device.h"

#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <set>

#include "DrawCmd.h"

#define FRAME_LAG 2

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
  CreateInstance(window->window());
  CreateSurface(window->window());
  SelectPhysicalDevice();
  SelectQueueIndex();
  CreateDevice();
  GetQueues();
  CreateSwapchainResource(VK_NULL_HANDLE);
  CreateSyncObject();
}

Device::~Device() {
  device_.waitIdle();
  if (device_) {
    if (swapchain_.handle) {
      device_.destroy(swapchain_.handle);
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

  for (uint32_t i = 0; i < swapbuffers_.count; i++) {
    device_.waitForFences(1, &fences_[i], VK_TRUE, UINT64_MAX);
  }

  DestroySwapchainResource();
  vk::SwapchainKHR oldSwapchain = swapchain_.handle;
  CreateSwapchainResource(oldSwapchain);

  device_.destroy(oldSwapchain);
}

void Device::InitRenderPath(ShaderPass* path) {
  path->CreateRenderPass(swapchain_.image_format, depth_.format);
  path->CreateFrameBuffer(swapchain_.image_extent, swapbuffers_.count,
                          swapbuffers_.imageviews.get(), depth_.imageview);
}

void Device::PrepareRender() {
  device_.waitForFences(1, &fences_[0], VK_TRUE, UINT64_MAX);
  device_.resetFences(1, &fences_[0]);

  auto& curBuf = current_buffer_;

  vk::Result result;
  do {
    result = device_.acquireNextImageKHR(swapchain_.handle, UINT64_MAX,
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
}

void Device::Render(DrawParam* param) {

  param->Render(commands_[current_buffer_], current_buffer_);
}

void Device::FinishRender() {
  vk::PipelineStageFlags pipeStageFlags =
      vk::PipelineStageFlagBits::eColorAttachmentOutput;
  auto const submitInfo =
      vk::SubmitInfo()
          .setPWaitDstStageMask(&pipeStageFlags)
          .setWaitSemaphoreCount(1)
          .setPWaitSemaphores(&image_acquired_[frame_index_])
          .setCommandBufferCount(1)
          .setPCommandBuffers(&commands_[current_buffer_])
          .setSignalSemaphoreCount(1)
          .setPSignalSemaphores(&render_complete_[frame_index_]);
  vk::Result result;
  result = queues_.graphics.submit(1, &submitInfo, fences_[0]);
  assert(result == vk::Result::eSuccess);

  auto const presentInfo =
      vk::PresentInfoKHR()
          .setWaitSemaphoreCount(1)
          .setPWaitSemaphores(&render_complete_[frame_index_])
          .setSwapchainCount(1)
          .setPSwapchains(&swapchain_.handle)
          .setPImageIndices(&current_buffer_);

  result = queues_.present.presentKHR(&presentInfo);
  frame_index_ += 1;
  frame_index_ %= swapbuffers_.count;
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

  for (uint32_t i = 0; i < swapbuffers_.count; i++) {
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

  std::vector<uint32_t> indices = {indices_.graphics};
  if (indices_.graphics == indices_.present) {
    swapCI.setImageSharingMode(vk::SharingMode::eExclusive);
  } else {
    swapCI.setImageSharingMode(vk::SharingMode::eConcurrent);
    indices.push_back(indices_.present);
  }
  swapCI.setQueueFamilyIndices(indices);

  swapCI.setOldSwapchain(oldSwapchain);

  result = device_.createSwapchainKHR(&swapCI, nullptr, &swapchain_.handle);
  assert(result == vk::Result::eSuccess);

  swapchain_.image_extent = caps.currentExtent;
  swapchain_.image_format = surfaceFormat[0].format;

  CreateSwapchainImageViews(surfaceFormat[0].format);
  CreateDepthbuffer(caps.currentExtent);
  CreateCommandBuffers();
}

void Device::DestroySwapchainResource() {
  for (uint32_t i = 0; i < swapbuffers_.count; ++i) {
    if (swapbuffers_.imageviews[i]) {
      device_.destroy(swapbuffers_.imageviews[i]);
    }
  }
  if (command_pool_) {
    device_.destroy(command_pool_);
  }
  if (once_cmd_pool_) {
    device_.destroy(once_cmd_pool_);
  }

  if (depth_.image) {
    device_.destroy(depth_.image);
  }

  if (depth_.imageview) {
    device_.destroy(depth_.imageview);
  }

  if (depth_.memory) {
    device_.free(depth_.memory);
  }
}

void Device::CreateSwapchainImageViews(vk::Format format) {
  vk::Result result = vk::Result::eSuccess;

  result = device_.getSwapchainImagesKHR(swapchain_.handle, &swapbuffers_.count,
                                         nullptr);
  assert(result == vk::Result::eSuccess);
  swapbuffers_.images = std::make_unique<vk::Image[]>(swapbuffers_.count);
  result = device_.getSwapchainImagesKHR(swapchain_.handle, &swapbuffers_.count,
                                         swapbuffers_.images.get());
  assert(result == vk::Result::eSuccess);

  auto resRange = vk::ImageSubresourceRange()
                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                      .setLayerCount(1)
                      .setBaseArrayLayer(0)
                      .setLevelCount(1)
                      .setBaseMipLevel(0);

  swapbuffers_.imageviews =
      std::make_unique<vk::ImageView[]>(swapbuffers_.count);

  for (uint32_t i = 0; i < swapbuffers_.count; i++) {
    vk::ImageViewCreateInfo imageViewCI =
        vk::ImageViewCreateInfo()
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(format)
            .setPNext(nullptr)
            .setSubresourceRange(resRange);
    imageViewCI.setImage(swapbuffers_.images[i]);
    result = device_.createImageView(&imageViewCI, nullptr,
                                     &swapbuffers_.imageviews[i]);
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

  std::vector<uint32_t> indices = {indices_.graphics};
  if (indices_.graphics == indices_.present) {
    imageCI.setSharingMode(vk::SharingMode::eExclusive);
  } else {
    imageCI.setSharingMode(vk::SharingMode::eConcurrent);
    indices.push_back(indices_.present);
  }
  imageCI.setQueueFamilyIndices(indices);
  result = device_.createImage(&imageCI, nullptr, &depth_.image);
  assert(result == vk::Result::eSuccess);
  depth_.format = vk::Format::eD16Unorm;

  vk::MemoryAllocateInfo memoryAI;
  vk::MemoryRequirements memReq;
  device_.getImageMemoryRequirements(depth_.image, &memReq);
  memoryAI.setAllocationSize(memReq.size);
  auto pass = FindMemoryType(memReq.memoryTypeBits,
                             vk::MemoryPropertyFlagBits::eDeviceLocal,
                             memoryAI.memoryTypeIndex);
  assert(pass);
  result = device_.allocateMemory(&memoryAI, nullptr, &depth_.memory);
  assert(result == vk::Result::eSuccess);

  device_.bindImageMemory(depth_.image, depth_.memory, 0);
  auto imageViewCI = vk::ImageViewCreateInfo()
                         .setImage(depth_.image)
                         .setViewType(vk::ImageViewType::e2D)
                         .setFormat(vk::Format::eD16Unorm)
                         .setSubresourceRange(vk::ImageSubresourceRange(
                             vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1))
                         .setPNext(nullptr);
  result = device_.createImageView(&imageViewCI, nullptr, &depth_.imageview);
  assert(result == vk::Result::eSuccess);
}

void Device::CreateCommandBuffers() {
  vk::Result result;

  auto cmdPoolCI =
      vk::CommandPoolCreateInfo()
          .setQueueFamilyIndex(indices_.graphics)
          .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
  result = device_.createCommandPool(&cmdPoolCI, nullptr, &command_pool_);
  assert(result == vk::Result::eSuccess);

  cmdPoolCI.setFlags(vk::CommandPoolCreateFlagBits::eTransient);
  result = device_.createCommandPool(&cmdPoolCI, nullptr, &once_cmd_pool_);
  assert(result == vk::Result::eSuccess);

  auto cmdAI = vk::CommandBufferAllocateInfo()
                   .setCommandPool(command_pool_)
                   .setLevel(vk::CommandBufferLevel::ePrimary)
                   .setCommandBufferCount(1);

  commands_ = std::make_unique<vk::CommandBuffer[]>(swapbuffers_.count);
  for (size_t i = 0; i < swapbuffers_.count; i++) {
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

void Device::SelectPhysicalDevice() {
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

  property_ = gpu_.getProperties();
}

void Device::SelectQueueIndex() {
  auto queueProperties = gpu_.getQueueFamilyProperties();
  uint32_t indexCount = (uint32_t)queueProperties.size();

  std::vector<vk::Bool32> supportsPresent(indexCount);
  for (uint32_t i = 0; i < indexCount; i++) {
    gpu_.getSurfaceSupportKHR(i, surface_, &supportsPresent[i]);
  }

  for (uint32_t i = 0; i < indexCount; ++i) {
    if (queueProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
      indices_.graphics = i;
    }

    if (supportsPresent[i] == VK_TRUE) {
      indices_.present = i;
    }

    if (indices_.graphics != UINT32_MAX && indices_.present != UINT32_MAX) {
      break;
    }
  }
  assert(indices_.graphics != UINT32_MAX && indices_.present != UINT32_MAX);
}

void Device::CreateDevice() {
  vk::Result result = vk::Result::eSuccess;

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
  std::set<uint32_t> queueFamilies = {indices_.graphics, indices_.present};

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
  queues_.graphics = device_.getQueue(indices_.graphics, 0);
  queues_.present = device_.getQueue(indices_.present, 0);
}

void Device::CreateSyncObject() {
  vk::Result result;

  frame_count_ = FRAME_LAG;

  auto semaphoreCreateInfo = vk::SemaphoreCreateInfo();
  auto fenceCI =
      vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);

  fences_ = std::make_unique<vk::Fence[]>(frame_count_);
  image_acquired_ = std::make_unique<vk::Semaphore[]>(frame_count_);
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

StageBuffer::StageBuffer(DeviceResource* dst, const void* data, size_t size)
    : DeviceResource(dst->GetParent()), size_(size) {
  do {
    buffer_ = CreateBuffer(vk::BufferUsageFlagBits::eTransferSrc, size_);
    if (!buffer_) break;

    memory_ = CreateMemory(device().getBufferMemoryRequirements(buffer_),
                           vk::MemoryPropertyFlagBits::eHostVisible |
                               vk::MemoryPropertyFlagBits::eHostCoherent);

    if (!memory_) break;

    device().bindBufferMemory(buffer_, memory_, 0);
    auto* mapData = device().mapMemory(memory_, 0, size_);
    memcpy(mapData, data, size_);
    device().unmapMemory(memory_);
  } while (false);
}

StageBuffer::~StageBuffer() {
  if (buffer_) {
    device().destroy(buffer_);
  }
  if (memory_) {
    device().free(memory_);
  }
}

bool StageBuffer::CopyToBuffer(const vk::Buffer& dstBuffer) {
  CommandOnce cmd(this);
  if (!cmd) return false;

  cmd.CopyBuffer2Buffer(buffer_, dstBuffer, size_);
  return true;
}

bool StageBuffer::CopyToImage(const vk::Image& dstImage, uint32_t width,
                              uint32_t height, uint32_t channel) {
  CommandOnce cmd(this);
  if (!cmd) return false;

  cmd.CopyBuffer2Image(buffer_, dstImage, width, height, channel);
  return true;
}

DeviceResource::DeviceResource(Device* parent) : parent_(parent) {}

DeviceResource::~DeviceResource() {}

} // namespace impl
} // namespace VPP
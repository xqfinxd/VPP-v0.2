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
  CreateCommandPools();
}

Device::~Device() {
  device_.waitIdle();
  if (device_) {
    if (reset_command_pool_) {
      device_.destroy(reset_command_pool_);
    }
    if (once_command_pool_) {
      device_.destroy(once_command_pool_);
    }
    if (swapchain_) {
      DestroySwapchainObject(swapchain_);
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

  DestroyDepthbuffer();
  CreateSwapchainObject(swapchain_, vk::ImageUsageFlagBits::eTransferDst);
}

void Device::Draw() {
  device_.waitForFences(1, &fences_[0], VK_TRUE, UINT64_MAX);
  device_.resetFences(1, &fences_[0]);

  
}

void Device::EndDraw() {
  device_.waitIdle();

  
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

  CreateSyncObject();
  CreateDepthbuffer(curExtent);
}

void Device::DestroySwapchainObject(SwapchainObject& swapchain) {
  device_.destroy(swapchain.object);
  swapchain.object = VK_NULL_HANDLE;
  DestroyDepthbuffer();
}

void Device::DestroyDepthbuffer() {
  if (depth_.image) {
    device_.destroy(depth_.image);
  }

  if (depth_.memory) {
    device_.free(depth_.memory);
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
  imageCI.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
  imageCI.setFormat(vk::Format::eD16Unorm);
  result = device_.createImage(&imageCI, nullptr, &depth_.image);
  assert(result == vk::Result::eSuccess);

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
}

void Device::CreateCommandPools() {
  vk::Result result;

  auto cmdPoolCI =
      vk::CommandPoolCreateInfo()
          .setQueueFamilyIndex(graphics_.index)
          .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
  result = device_.createCommandPool(&cmdPoolCI, nullptr, &reset_command_pool_);
  assert(result == vk::Result::eSuccess);

  cmdPoolCI.setFlags((vk::CommandPoolCreateFlags)0);
  result = device_.createCommandPool(&cmdPoolCI, nullptr, &once_command_pool_);
  assert(result == vk::Result::eSuccess);
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

    if (queueProperties[i].queueFlags & vk::QueueFlagBits::eGraphics &&
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

  auto semaphoreCreateInfo = vk::SemaphoreCreateInfo();
  auto fenceCI =
      vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);

  fences_          = std::make_unique<vk::Fence[]>(swapchain_.ImageCount());
  image_acquired_  = std::make_unique<vk::Semaphore[]>(swapchain_.ImageCount());
  render_complete_ = std::make_unique<vk::Semaphore[]>(swapchain_.ImageCount());

  for (uint32_t i = 0; i < swapchain_.ImageCount(); i++) {
    result = device_.createFence(&fenceCI, nullptr, &fences_[i]);
    assert(result == vk::Result::eSuccess);

    result = device_.createSemaphore(&semaphoreCreateInfo, nullptr,
                                     &image_acquired_[i]);
    assert(result == vk::Result::eSuccess);

    result = device_.createSemaphore(&semaphoreCreateInfo, nullptr,
                                     &render_complete_[i]);
    assert(result == vk::Result::eSuccess);
  }
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

bool DeviceResource::CopyPresent(const vk::Image& to) const {
  
  return true;
}

vk::CommandBuffer DeviceResource::BeginOnceCmd() const {
  auto cmdAI = vk::CommandBufferAllocateInfo()
                   .setCommandPool(parent_->reset_command_pool_)
                   .setLevel(vk::CommandBufferLevel::ePrimary)
                   .setCommandBufferCount(1);

  vk::CommandBuffer cmd;
  if (device().allocateCommandBuffers(&cmdAI, &cmd) != vk::Result::eSuccess) {
    return VK_NULL_HANDLE;
  }

  auto beginInfo = vk::CommandBufferBeginInfo().setFlags(
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  cmd.begin(beginInfo);
  return cmd;
}

void DeviceResource::EndOnceCmd(vk::CommandBuffer& cmd) const {
  cmd.end();

  parent_->transfer_.queue.waitIdle();
  auto submitInfo =
      vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&cmd);
  parent_->transfer_.queue.submit(1, &submitInfo, VK_NULL_HANDLE);
  parent_->transfer_.queue.waitIdle();

  device().free(parent_->reset_command_pool_, 1, &cmd);
  cmd = VK_NULL_HANDLE;
}

StageBuffer::StageBuffer(Device* parent, const void* data, size_t size)
    : DeviceResource(parent) {
  buffer_ = CreateBuffer(vk::BufferUsageFlagBits::eTransferSrc, size);
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
  auto* mapData = device().mapMemory(memory_, 0, size);
  memcpy(mapData, data, size);
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

bool StageBuffer::CopyToBuffer(const vk::Buffer& dstBuffer, size_t size) {
  if (!buffer_ || !memory_) {
    return false;
  }
  auto cmd = BeginOnceCmd();
  if (!cmd) {
    return false;
  }

  auto copyRegion =
      vk::BufferCopy().setDstOffset(0).setSrcOffset(0).setSize(size);
  cmd.copyBuffer(buffer_, dstBuffer, 1, &copyRegion);

  EndOnceCmd(cmd);
  return true;
}

bool StageBuffer::CopyToImage(const vk::Image& dstImage, uint32_t width,
                         uint32_t height, uint32_t channel) {
  auto cmd = BeginOnceCmd();
  if (!cmd) {
    return false;
  }
  
  { // to transfer pipeline stage
    auto barrier = vk::ImageMemoryBarrier()
                       .setOldLayout(vk::ImageLayout::eUndefined)
                       .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                       .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setImage(dstImage)
                       .setSubresourceRange(vk::ImageSubresourceRange{
                           vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
                       .setSrcAccessMask((vk::AccessFlags)0)
                       .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                        vk::PipelineStageFlagBits::eTransfer,
                        (vk::DependencyFlagBits)0, 0, nullptr, 0, nullptr, 1,
                        &barrier);
  }

  auto region = vk::BufferImageCopy()
                    .setBufferOffset(0)
                    .setBufferRowLength(width)
                    .setBufferImageHeight(height)
                    .setImageSubresource(vk::ImageSubresourceLayers{
                        vk::ImageAspectFlagBits::eColor, 0, 0, 1})
                    .setImageOffset(vk::Offset3D{0, 0, 0})
                    .setImageExtent(vk::Extent3D{width, height, 1});
  cmd.copyBufferToImage(buffer_, dstImage,
                        vk::ImageLayout::eTransferDstOptimal, 1, &region);
  
  { // to shader pipeline stage
    auto barrier = vk::ImageMemoryBarrier()
                       .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                       .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                       .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                       .setImage(dstImage)
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

  EndOnceCmd(cmd);
}

} // namespace impl
} // namespace VPP
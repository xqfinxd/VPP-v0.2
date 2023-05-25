#include "renderer_impl.h"

#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <set>

#include "window_impl.h"

#define SAFE_DESTROY(p, o) \
  if (o)                   \
  (p).destroy(o)
#define SAFE_FREE(p, o) \
  if (o)                \
  p.free(o)

namespace VPP {

namespace impl {

const std::vector<const char*> g_Extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
const std::vector<const char*> g_Layers = {"VK_LAYER_KHRONOS_validation"};

struct RenderSupport {
  vk::SurfaceCapabilitiesKHR                   capabilities{};
  std::vector<vk::SurfaceFormatKHR>            formats{};
  std::vector<vk::PresentModeKHR>              presentModes{};
  std::vector<vk::ExtensionProperties>         extensions{};
  uint32_t                                     indexCount = 0;
  std::unique_ptr<vk::QueueFamilyProperties[]> queueProperties{};
  std::unique_ptr<uint32_t[]>                  queuePresents{};

  RenderSupport& Query(const vk::PhysicalDevice& gpu,
                       const vk::SurfaceKHR&     surface) {
    gpu.getSurfaceCapabilitiesKHR(surface, &capabilities);
    formats = gpu.getSurfaceFormatsKHR(surface);
    presentModes = gpu.getSurfacePresentModesKHR(surface);

    extensions = gpu.enumerateDeviceExtensionProperties();

    gpu.getQueueFamilyProperties(&indexCount, nullptr);
    assert(indexCount > 0);
    queueProperties = std::make_unique<vk::QueueFamilyProperties[]>(indexCount);
    gpu.getQueueFamilyProperties(&indexCount, queueProperties.get());

    queuePresents = std::make_unique<uint32_t[]>(indexCount);
    for (uint32_t i = 0; i < indexCount; ++i) {
      queuePresents[i] = gpu.getSurfaceSupportKHR(i, surface);
    }

    return *this;
  }

  vk::SurfaceFormatKHR GetFormat() const {
    for (const auto& fmt : formats) {
      if (fmt.format == vk::Format::eB8G8R8A8Srgb &&
          fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
        return fmt;
      }
    }

    return formats[0];
  }

  vk::PresentModeKHR GetPresentMode() const {
    for (const auto& mode : presentModes) {
      if (mode == vk::PresentModeKHR::eMailbox) {
        return mode;
      }
    }

    return vk::PresentModeKHR::eFifo;
  }

  vk::Extent2D GetExtent() const {
    if (capabilities.currentExtent.width != UINT32_MAX &&
        capabilities.currentExtent.height != UINT32_MAX) {
      return capabilities.currentExtent;
    } else {
      return capabilities.minImageExtent;
    }
  }

  uint32_t GetMinImageCount() const {
    return capabilities.minImageCount;
  }

  QueueIndices GetIndices() const {
    QueueIndices indices{};
    for (uint32_t i = 0; i < indexCount; ++i) {
      if (queueProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
        indices.graphics = i;
      }

      if (queuePresents[i] == VK_TRUE) {
        indices.present = i;
      }

      if (indices.HasValue()) {
        break;
      }
    }
    return indices;
  }

  bool Check(const std::vector<const char*>& reqExtensions) const {
    std::set<std::string> extensionList(reqExtensions.begin(),
                                        reqExtensions.end());
    for (const auto& ext : extensions) {
      extensionList.erase(ext.extensionName);
    }
    bool extensionsOk = extensionList.empty();

    bool surfaceOk = false;
    if (extensionsOk) {
      surfaceOk = !formats.empty() && !presentModes.empty();
    }

    auto indices = GetIndices();

    return extensionsOk && surfaceOk && indices.HasValue();
  }
};

static VkBool32 DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      level,
    VkDebugUtilsMessageTypeFlagsEXT             type,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                       pUserData) {
  switch (level) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      fprintf(stderr, "[vulkan] Info: %s\n", pCallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      fprintf(stderr, "[vulkan] Warn: %s\n", pCallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      fprintf(stderr, "[vulkan] Error: %s\n", pCallbackData->pMessage);
      break;
    default:
      break;
  }

  return VK_FALSE;
}

static std::vector<const char*> GetWindowExtensions(SDL_Window* window) {
  SDL_bool                 result = SDL_TRUE;
  std::vector<const char*> extensions{};
  uint32_t                 extCount = 0;
  result = SDL_Vulkan_GetInstanceExtensions(window, &extCount, nullptr);
  extensions.resize(extCount);
  result =
      SDL_Vulkan_GetInstanceExtensions(window, &extCount, extensions.data());
  return extensions;
}

Renderer::Renderer() {}

Renderer::~Renderer() {
  Clean();
}

void Renderer::SetupRenderer() {
  vk::Result result;

  {  // instance
    auto appCI = vk::ApplicationInfo()
                     .setPNext(nullptr)
                     .setPApplicationName("Vulkan Engine")
                     .setApplicationVersion(0)
                     .setApiVersion(VK_API_VERSION_1_1)
                     .setPEngineName("None")
                     .setEngineVersion(0);

    auto& wnd = Window::GetMe();
    auto  extensions = GetWindowExtensions(wnd.window_);
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
                      .setPEnabledLayerNames(g_Layers)
                      .setPEnabledExtensionNames(extensions)
                      .setPApplicationInfo(&appCI)
                      .setPNext(&debugCI);

    result = vk::createInstance(&instCI, nullptr, &instance_);
    assert(result == vk::Result::eSuccess);
  }
  {  // surface
    VkSurfaceKHR cSurf;
    auto&        wnd = Window::GetMe();
    SDL_Vulkan_CreateSurface(wnd.window_, instance_, &cSurf);
    surface_ = cSurf;
    assert(surface_);
  }
  {  // physical device
    auto          availableGPUs = instance_.enumeratePhysicalDevices();
    bool          found = false;
    RenderSupport support{};
    for (const auto& e : availableGPUs) {
      support.Query(e, surface_);
      if (support.Check(g_Extensions)) {
        gpu_ = e;
        found = true;
        break;
      }
    }
    assert(found);
    auto extent = support.GetExtent();
    context.width = extent.width;
    context.height = extent.height;
    context.surface_format = support.GetFormat();
    context.present_mode = support.GetPresentMode();
    context.min_image_count = support.GetMinImageCount();

    indices_ = support.GetIndices();
  }
  {  // device
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

    vk::DeviceCreateInfo deviceCI = vk::DeviceCreateInfo()
                                        .setQueueCreateInfoCount(1)
                                        .setQueueCreateInfos(queueCreateInfos)
                                        .setPEnabledExtensionNames(g_Extensions)
                                        .setPEnabledLayerNames(g_Layers)
                                        .setPEnabledFeatures(nullptr);

    result = gpu_.createDevice(&deviceCI, nullptr, &device_);
    assert(result == vk::Result::eSuccess);
  }
  {  // queue
    queues_.graphics = device_.getQueue(indices_.graphics, 0);
    queues_.present = device_.getQueue(indices_.present, 0);
  }
}

void Renderer::SetupContext(RenderContext& ctx) {
  vk::Result result;

  {  // swapchain
    auto info = vk::SwapchainCreateInfoKHR()
                    .setImageArrayLayers(1)
                    .setClipped(true)
                    .setSurface(surface_)
                    .setMinImageCount(ctx.min_image_count)
                    .setImageFormat(ctx.surface_format.format)
                    .setImageColorSpace(ctx.surface_format.colorSpace)
                    .setImageExtent(vk::Extent2D(ctx.width, ctx.height))
                    .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                    .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
                    .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                    .setPresentMode(ctx.present_mode);

    auto indicesData = indices_.Pack();

    if (indicesData.size() > 1) {
      info.setImageSharingMode(vk::SharingMode::eConcurrent);
    } else {
      info.setImageSharingMode(vk::SharingMode::eExclusive);
    }
    info.setQueueFamilyIndices(indicesData);

    result = device_.createSwapchainKHR(&info, nullptr, &ctx.swapchain);
    assert(result == vk::Result::eSuccess);
  }
  {  // depth buffer
    auto imageCI =
        vk::ImageCreateInfo()
            .setFormat(vk::Format::eD16Unorm)
            .setImageType(vk::ImageType::e2D)
            .setExtent(vk::Extent3D(ctx.width, ctx.height, 1u))
            .setMipLevels(1)
            .setArrayLayers(1)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
            .setInitialLayout(vk::ImageLayout::eUndefined);
    auto indicesData = indices_.Pack();
    if (indices_.graphics != indices_.present) {
      indicesData.push_back(indices_.present);
      imageCI.setSharingMode(vk::SharingMode::eConcurrent);
    } else {
      imageCI.setSharingMode(vk::SharingMode::eExclusive);
    }
    imageCI.setQueueFamilyIndices(indicesData);
    result = device_.createImage(&imageCI, nullptr, &ctx.depthbuffer);
    assert(result == vk::Result::eSuccess);

    vk::MemoryAllocateInfo memoryAI;
    vk::MemoryRequirements memReq;
    device_.getImageMemoryRequirements(ctx.depthbuffer, &memReq);
    memoryAI.setAllocationSize(memReq.size);
    auto pass = FindMemoryType(memReq.memoryTypeBits,
                               vk::MemoryPropertyFlagBits::eDeviceLocal,
                               memoryAI.memoryTypeIndex);
    assert(pass);
    result =
        device_.allocateMemory(&memoryAI, nullptr, &ctx.depthbuffer_memory);
    assert(result == vk::Result::eSuccess);

    device_.bindImageMemory(ctx.depthbuffer, ctx.depthbuffer_memory, 0);
    auto imageViewCI = vk::ImageViewCreateInfo()
                           .setImage(ctx.depthbuffer)
                           .setViewType(vk::ImageViewType::e2D)
                           .setFormat(vk::Format::eD16Unorm)
                           .setSubresourceRange(vk::ImageSubresourceRange(
                               vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1))
                           .setPNext(nullptr);
    result =
        device_.createImageView(&imageViewCI, nullptr, &ctx.depthbuffer_view);
    assert(result == vk::Result::eSuccess);
  }
  {  // render pass
    std::vector<vk::AttachmentDescription> attachments = {
        vk::AttachmentDescription()
            .setFormat(ctx.surface_format.format)
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

    result = device_.createRenderPass(&renderPassCI, nullptr, &ctx.render_pass);
    assert(result == vk::Result::eSuccess);
  }
  {  // command pool
    auto info =
        vk::CommandPoolCreateInfo().setQueueFamilyIndex(indices_.graphics);
    result = device_.createCommandPool(&info, nullptr, &ctx.command_pool);
    assert(result == vk::Result::eSuccess);
  }
  {  // clear
    ctx.enable_clear_buffer = true;
    ctx.clear_value.setColor({0.0f, 0.0f, 0.0f, 1.0f});
    ctx.image_count = 0;
    ctx.frame_index = 0;
    ctx.semaphore_index = 0;
  }
  {  // frame
    auto images = device_.getSwapchainImagesKHR(ctx.swapchain);
    assert(images.size() > 0);
    ctx.image_count = (uint32_t)images.size();

    ctx.frames = std::make_unique<RenderFrame[]>(ctx.image_count);
    for (uint32_t i = 0; i < ctx.image_count; ++i) {
      ctx.frames[i].colorbuffer = images[i];
      CreateFrame(ctx, ctx.frames[i]);
    }

    ctx.semaphores = std::make_unique<FrameSemaphores[]>(ctx.image_count);
    for (uint32_t i = 0; i < ctx.image_count; ++i) {
      CreateSemaphore(ctx, ctx.semaphores[i]);
    }
  }
}

void Renderer::DestroyFrame(RenderFrame& frame) {
  SAFE_DESTROY(device_, frame.framebuffer);
  SAFE_DESTROY(device_, frame.colorbuffer_view);
  SAFE_DESTROY(device_, frame.fence);
}

void Renderer::DestroySemaphore(FrameSemaphores& semaphore) {
  SAFE_DESTROY(device_, semaphore.image_acquired);
  SAFE_DESTROY(device_, semaphore.render_complete);
}

void Renderer::CreateFrame(const RenderContext& ctx, RenderFrame& frame) {
  vk::Result result;

  {  // color buffer view
    auto resRange = vk::ImageSubresourceRange()
                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                        .setLayerCount(1)
                        .setBaseArrayLayer(0)
                        .setLevelCount(1)
                        .setBaseMipLevel(0);

    vk::ImageViewCreateInfo imageViewCI =
        vk::ImageViewCreateInfo()
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(ctx.surface_format.format)
            .setPNext(nullptr)
            .setSubresourceRange(resRange);
    imageViewCI.setImage(frame.colorbuffer);
    result =
        device_.createImageView(&imageViewCI, nullptr, &frame.colorbuffer_view);
    assert(result == vk::Result::eSuccess);
  }
  {  // frame buffer
    vk::ImageView attachments[2];
    attachments[1] = ctx.depthbuffer_view;

    auto frameBufferCI = vk::FramebufferCreateInfo()
                             .setRenderPass(ctx.render_pass)
                             .setAttachmentCount(2)
                             .setPAttachments(attachments)
                             .setWidth(ctx.width)
                             .setHeight(ctx.height)
                             .setLayers(1);

    attachments[0] = frame.colorbuffer_view;
    auto result =
        device_.createFramebuffer(&frameBufferCI, nullptr, &frame.framebuffer);
    assert(result == vk::Result::eSuccess);
  }
  {  // command buffer
    auto info = vk::CommandBufferAllocateInfo()
                    .setCommandPool(ctx.command_pool)
                    .setLevel(vk::CommandBufferLevel::ePrimary)
                    .setCommandBufferCount(1);

    result = device_.allocateCommandBuffers(&info, &frame.command_buffer);
    assert(result == vk::Result::eSuccess);
  }
  {  // fence
    auto const fenceCI =
        vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
    result = device_.createFence(&fenceCI, nullptr, &frame.fence);
    assert(result == vk::Result::eSuccess);
  }
}
void Renderer::CreateSemaphore(const RenderContext& ctx,
                               FrameSemaphores&     semaphore) {
  vk::Result result;

  auto semaphoreCreateInfo = vk::SemaphoreCreateInfo();
  result = device_.createSemaphore(&semaphoreCreateInfo, nullptr,
                                   &semaphore.image_acquired);
  assert(result == vk::Result::eSuccess);

  result = device_.createSemaphore(&semaphoreCreateInfo, nullptr,
                                   &semaphore.render_complete);
  assert(result == vk::Result::eSuccess);
}

void Renderer::CleanRenderer() {
  device_.destroy();
  SAFE_DESTROY(instance_, surface_);
  instance_.destroy();
}

void Renderer::Setup() {
  SetupRenderer();
  SetupContext(context);
}

void Renderer::Clean() {
  CleanContext(context);
  CleanRenderer();
}

bool Renderer::FindMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask,
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

void Renderer::CleanContext(RenderContext& ctx) {
  SAFE_DESTROY(device_, ctx.command_pool);
  SAFE_DESTROY(device_, ctx.render_pass);
  SAFE_FREE(device_, ctx.depthbuffer_memory);
  SAFE_DESTROY(device_, ctx.depthbuffer_view);
  SAFE_DESTROY(device_, ctx.depthbuffer);
  SAFE_DESTROY(device_, ctx.swapchain);
  for (size_t i = 0; i < ctx.image_count; ++i) {
    DestroyFrame(ctx.frames[i]);
    DestroySemaphore(ctx.semaphores[i]);
  }
}

}  // namespace impl

}  // namespace VPP

#undef SAFE_DESTROY
#undef SAFE_FREE
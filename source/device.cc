#include "device.h"

#include <GLFW/glfw3.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <glm/glm.hpp>
#include <iostream>
#include <set>
#include <spirv-headers/spirv.hpp>
#include <vector>

#include "window.h"

static const std::vector<const char*> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation"};

static const std::vector<const char*> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static VkBool32 DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT level,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
  switch (level) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      std::cerr << "\033[1;37mVK Log: ";
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      std::cerr << "\033[1;33mVK Warning: ";
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      std::cerr << "\033[1;31mVK Error: ";
      break;
    default:
      std::cerr << "\033[0m VK: ";
      break;
  }

  std::cerr << "\033[0m" << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

QueueIndices::QueueIndices(const vk::PhysicalDevice& gpu,
                           const vk::SurfaceKHR& surface) {
  auto queueFamilies = gpu.getQueueFamilyProperties();

  uint32_t i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
      graphics = i;

    auto presentSupport = gpu.getSurfaceSupportKHR(i, surface);
    if (presentSupport)
      present = i;

    if (valid())
      break;

    i++;
  }
}

SurfaceSupport::SurfaceSupport(const vk::PhysicalDevice& gpu,
                               const vk::SurfaceKHR& surface) {
  gpu.getSurfaceCapabilitiesKHR(surface, &capabilities);
  formats = gpu.getSurfaceFormatsKHR(surface);
  presentModes = gpu.getSurfacePresentModesKHR(surface);
}

vk::SurfaceFormatKHR SurfaceSupport::selectFormat() const {
  for (const auto& availableFormat : formats) {
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return availableFormat;
    }
  }

  return formats[0];
}

vk::PresentModeKHR SurfaceSupport::selectPresentMode() const {
  for (const auto& mode : presentModes) {
    if (mode == vk::PresentModeKHR::eMailbox) {
      return mode;
    }
  }

  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D SurfaceSupport::selectExtent() const {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  } else {
    auto actualExtent = MainWindow::getMe().getSurfExtent();

    actualExtent.width =
        glm::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        glm::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

uint32_t SurfaceSupport::selectImageCount() const {
  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 &&
      imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }
  return imageCount;
}

vk::CompositeAlphaFlagBitsKHR SurfaceSupport::selectAlpha() const {
  auto alpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  std::vector<vk::CompositeAlphaFlagBitsKHR> allAlphas = {
      vk::CompositeAlphaFlagBitsKHR::eOpaque,
      vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
      vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
      vk::CompositeAlphaFlagBitsKHR::eInherit,
  };
  for (const auto& e : allAlphas) {
    if (capabilities.supportedCompositeAlpha & e)
      return e;
  }
  return alpha;
}

Renderer::Renderer() {
  initInstance();
  initSurface();
  initGpu();
  initQueueIndices();
  initDevice();
  initQueue();
  initSwapchain();
  initSwapImages();
  initDepthImage();
  initRenderPass();
  initFramebuffer();
}

Renderer::~Renderer() {
  device.destroy(renderPass);
  device.destroy(depthImage.view);
  device.destroy(depthImage.image);
  device.free(depthImage.memory);
  for (size_t i = 0; i < swapImageCount; i++) {
    device.destroy(swapImages[i].view);
    device.destroy(framebuffers[i]);
  }
  device.destroy(swapchain);
  device.destroy();
  instance.destroySurfaceKHR(surface);
  instance.destroy();
}

void Renderer::initInstance() {
  vk::Result result;

  auto appCI = vk::ApplicationInfo()
                   .setPNext(nullptr)
                   .setPApplicationName("Vulkan Engine")
                   .setApplicationVersion(0)
                   .setApiVersion(VK_API_VERSION_1_1)
                   .setPEngineName("None")
                   .setEngineVersion(0);

  auto extensions = MainWindow::getMe().getExtensions();
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
                    .setPEnabledLayerNames(kValidationLayers)
                    .setPEnabledExtensionNames(extensions)
                    .setPApplicationInfo(&appCI)
                    .setPNext(&debugCI);

  result = vk::createInstance(&instCI, nullptr, &instance);
  IFNO_THROW(result == vk::Result::eSuccess, "fail to create instance");
}

void Renderer::initSurface() {
  surface = MainWindow::getMe().getSurface(instance);
  IFNO_THROW(surface, "fail to create surface");
}

bool Renderer::checkPhysicalDevice(const vk::PhysicalDevice& gpu) const {
  QueueIndices gpuIndices(gpu, surface);

  auto availableExtensions = gpu.enumerateDeviceExtensionProperties();
  std::set<std::string> requiredExtensions(kDeviceExtensions.begin(),
                                           kDeviceExtensions.end());
  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }
  bool extensionsSupported = requiredExtensions.empty();

  bool swapchainAdequate = false;
  if (extensionsSupported) {
    SurfaceSupport surfSupport(gpu, surface);
    swapchainAdequate =
        !(surfSupport.formats.empty() || surfSupport.presentModes.empty());
  }

  return gpuIndices && extensionsSupported && swapchainAdequate;
}

void Renderer::initGpu() {
  auto availableGPUs = instance.enumeratePhysicalDevices();
  bool found = false;
  for (const auto& e : availableGPUs) {
    if (checkPhysicalDevice(e)) {
      gpu = e;
      found = true;
      break;
    }
  }
  IFNO_THROW(found, "fail to find suitable gpu");
}

void Renderer::initQueueIndices() {
  indices = QueueIndices(gpu, surface);
  IFNO_THROW(indices.valid(), "fail to find suitable queue indices");
}

void Renderer::initDevice() {
  vk::Result result;

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
  std::set<uint32_t> queueFamilies = {indices.graphics, indices.present};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : queueFamilies) {
    auto queueCreateInfo = vk::DeviceQueueCreateInfo()
                               .setQueueFamilyIndex(queueFamily)
                               .setQueueCount(1)
                               .setPQueuePriorities(&queuePriority);
    queueCreateInfos.push_back(queueCreateInfo);
  }

  vk::DeviceCreateInfo deviceCI =
      vk::DeviceCreateInfo()
          .setQueueCreateInfoCount(1)
          .setQueueCreateInfos(queueCreateInfos)
          .setPEnabledExtensionNames(kDeviceExtensions)
          .setPEnabledLayerNames(kValidationLayers)
          .setPEnabledFeatures(nullptr);

  result = gpu.createDevice(&deviceCI, nullptr, &device);
  IFNO_THROW(result == vk::Result::eSuccess, "fail to create device");
}

void Renderer::initQueue() {
  queues.graphics = device.getQueue(indices.graphics, 0);
  queues.present = device.getQueue(indices.present, 0);
}

void Renderer::initSwapchain() {
  vk::Result result;

  SurfaceSupport surfSupport(gpu, surface);
  auto presentMode = surfSupport.selectPresentMode();
  auto format = surfSupport.selectFormat();
  auto extent = surfSupport.selectExtent();
  auto imageCount = surfSupport.selectImageCount();
  auto alpha = surfSupport.selectAlpha();

  auto swapchainCI =
      vk::SwapchainCreateInfoKHR()
          .setPNext(nullptr)
          .setMinImageCount(imageCount)
          .setImageFormat(format.format)
          .setImageColorSpace(format.colorSpace)
          .setImageExtent(extent)
          .setImageArrayLayers(1)
          .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
          .setPreTransform(surfSupport.capabilities.currentTransform)
          .setCompositeAlpha(alpha)
          .setPresentMode(presentMode)
          .setClipped(true)
          .setSurface(surface);

  std::vector<uint32_t> swapIndices{indices.graphics};
  if (indices.graphics != indices.present) {
    swapIndices.push_back(indices.present);
    swapchainCI.setImageSharingMode(vk::SharingMode::eConcurrent);
  } else {
    swapchainCI.setImageSharingMode(vk::SharingMode::eExclusive);
  }
  swapchainCI.setQueueFamilyIndices(swapIndices);

  result = device.createSwapchainKHR(&swapchainCI, nullptr, &swapchain);
  IFNO_THROW(result == vk::Result::eSuccess, "fail to create swapchains");
}

void Renderer::initSwapImages() {
  vk::Result result;

  auto images = device.getSwapchainImagesKHR(swapchain);
  IFNO_THROW(images.size() > 0, "fail to get image from swapchain");
  swapImageCount = (uint32_t)images.size();

  SurfaceSupport surfSupport(gpu, surface);
  auto format = surfSupport.selectFormat();
  auto resRange = vk::ImageSubresourceRange()
                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                      .setLayerCount(1)
                      .setBaseArrayLayer(0)
                      .setLevelCount(1)
                      .setBaseMipLevel(0);

  swapImages = std::make_unique<SwapImage[]>(swapImageCount);
  for (size_t i = 0; i < swapImageCount; i++) {
    vk::ImageViewCreateInfo imageViewCI =
        vk::ImageViewCreateInfo()
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(format.format)
            .setPNext(nullptr)
            .setSubresourceRange(resRange);
    imageViewCI.setImage(images[i]);
    result = device.createImageView(&imageViewCI, nullptr, &swapImages[i].view);
    IFNO_THROW(result == vk::Result::eSuccess,
               "fail to create swap image view");

    swapImages[i].image = images[i];
  }
}

void Renderer::initDepthImage() {
  vk::Result result;
  SurfaceSupport surfSupport(gpu, surface);
  auto extent = surfSupport.selectExtent();

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
  std::vector<uint32_t> depthIndices{indices.graphics};
  if (indices.graphics != indices.present) {
    depthIndices.push_back(indices.present);
    imageCI.setSharingMode(vk::SharingMode::eConcurrent);
  } else {
    imageCI.setSharingMode(vk::SharingMode::eExclusive);
  }
  imageCI.setQueueFamilyIndices(depthIndices);
  result = device.createImage(&imageCI, nullptr, &depthImage.image);
  IFNO_THROW(result == vk::Result::eSuccess, "fail to create depth image");

  vk::MemoryAllocateInfo memoryAI;
  vk::MemoryRequirements memReq;
  device.getImageMemoryRequirements(depthImage.image, &memReq);
  memoryAI.setAllocationSize(memReq.size);
  auto pass = getMemoryType(memReq.memoryTypeBits,
                            vk::MemoryPropertyFlagBits::eDeviceLocal,
                            memoryAI.memoryTypeIndex);
  IFNO_THROW(pass, "fail to get memory type");
  result = device.allocateMemory(&memoryAI, nullptr, &depthImage.memory);
  IFNO_THROW(result == vk::Result::eSuccess, "fail to allocate memory");

  device.bindImageMemory(depthImage.image, depthImage.memory, 0);
  auto imageViewCI = vk::ImageViewCreateInfo()
                         .setImage(depthImage.image)
                         .setViewType(vk::ImageViewType::e2D)
                         .setFormat(vk::Format::eD16Unorm)
                         .setSubresourceRange(vk::ImageSubresourceRange(
                             vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1))
                         .setPNext(nullptr);
  result = device.createImageView(&imageViewCI, nullptr, &depthImage.view);
  IFNO_THROW(result == vk::Result::eSuccess, "fail to create depth image view");
}

void Renderer::initRenderPass() {
  vk::Result result;

  SurfaceSupport surfSupport(gpu, surface);
  auto format = surfSupport.selectFormat();

  std::vector<vk::AttachmentDescription> attachments = {
      vk::AttachmentDescription()
          .setFormat(format.format)
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

  result = device.createRenderPass(&renderPassCI, nullptr, &renderPass);
  IFNO_THROW(result == vk::Result::eSuccess, "fail to create render pass");
}

void Renderer::initFramebuffer() {
  SurfaceSupport surfSupport(gpu, surface);
  auto extent = surfSupport.selectExtent();
  framebuffers = std::make_unique<vk::Framebuffer[]>(swapImageCount);

  vk::ImageView attachments[2];
  attachments[1] = depthImage.view;

  auto frameBufferCI = vk::FramebufferCreateInfo()
                           .setRenderPass(renderPass)
                           .setAttachmentCount(2)
                           .setPAttachments(attachments)
                           .setWidth(extent.width)
                           .setHeight(extent.height)
                           .setLayers(1);

  for (uint32_t i = 0; i < swapImageCount; i++) {
    attachments[0] = swapImages[i].view;
    auto result =
        device.createFramebuffer(&frameBufferCI, nullptr, &framebuffers[i]);
    IFNO_THROW(result == vk::Result::eSuccess, "fail to create frame buffer");
  }
}

bool Renderer::getMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask,
                             uint32_t& typeIndex) const {
  if (!gpu) {
    return false;
  }
  auto props = gpu.getMemoryProperties();
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
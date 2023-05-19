#include "device.h"
#include "device_d.h"

#include <iostream>
#include <set>

#include <SDL2/SDL_vulkan.h>

#include "console.h"
#include "window_d.h"

namespace helper {
std::vector<uint32_t> QueueIndices::pack() const {
    std::vector<uint32_t> indices{};
    if (!good()) {
        return indices;
    } else {
        indices.push_back(graphics);
        if (graphics != present) {
            indices.push_back(present);
        }
    }
    return indices;
}

vk::SurfaceFormatKHR GpuAux::getFormat() const {
    for (const auto& fmt : formats) {
        if (fmt.format == vk::Format::eB8G8R8A8Srgb &&
            fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return fmt;
        }
    }

    return formats[0];
}

vk::PresentModeKHR GpuAux::getPresentMode() const {
    for (const auto& mode : presentModes) {
        if (mode == vk::PresentModeKHR::eMailbox) {
            return mode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D GpuAux::getExtent() const {
    if (capabilities.currentExtent.width != UINT32_MAX &&
        capabilities.currentExtent.height != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        return capabilities.minImageExtent;
    }
}

uint32_t GpuAux::getNumOfImage() const {
    uint32_t num = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && num > capabilities.maxImageCount) {
        num = capabilities.maxImageCount;
    }
    return num;
}

vk::CompositeAlphaFlagBitsKHR GpuAux::getAlpha() const {
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

bool GpuAux::check(const std::vector<const char*>& reqExtensions) const {
    std::set<std::string> extensionList(reqExtensions.begin(),
                                        reqExtensions.end());
    for (const auto& ext : extensions) {
        extensionList.erase(ext.extensionName);
    }
    bool extensionsOk = extensionList.empty();

    bool swapchainOk = false;
    if (extensionsOk) {
        swapchainOk = !formats.empty() && !presentModes.empty();
    }

    return indices.good() && extensionsOk && swapchainOk;
}

vk::Result GpuAux::createSwapchain(const vk::Device& device,
                                   const vk::SurfaceKHR& surface,
                                   vk::SwapchainKHR& swapchain) const {
    auto info = vk::SwapchainCreateInfoKHR()
                    .setImageArrayLayers(1)
                    .setClipped(true)
                    .setSurface(surface);

    auto presentMode = getPresentMode();
    auto format = getFormat();
    auto extent = getExtent();
    auto imageCount = getNumOfImage();
    auto alpha = getAlpha();

    info.setMinImageCount(imageCount)
        .setImageFormat(format.format)
        .setImageColorSpace(format.colorSpace)
        .setImageExtent(extent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setPreTransform(capabilities.currentTransform)
        .setCompositeAlpha(alpha)
        .setPresentMode(presentMode)
        .setClipped(true)
        .setSurface(surface);

    auto swapIndices = indices.pack();
    ;
    if (swapIndices.size() > 1) {
        info.setImageSharingMode(vk::SharingMode::eConcurrent);
    } else {
        info.setImageSharingMode(vk::SharingMode::eExclusive);
    }
    info.setQueueFamilyIndices(swapIndices);

    return device.createSwapchainKHR(&info, nullptr, &swapchain);
}

GpuAux GpuAux::Query(const vk::PhysicalDevice& gpu,
                     const vk::SurfaceKHR& surface) {
    GpuAux evaluator{};
    gpu.getSurfaceCapabilitiesKHR(surface, &evaluator.capabilities);
    evaluator.formats = gpu.getSurfaceFormatsKHR(surface);
    evaluator.presentModes = gpu.getSurfacePresentModesKHR(surface);
    evaluator.extensions = gpu.enumerateDeviceExtensionProperties();

    uint32_t i = 0;
    auto queueFamilies = gpu.getQueueFamilyProperties();
    for (const auto& e : queueFamilies) {
        if (e.queueFlags & vk::QueueFlagBits::eGraphics) {
            evaluator.indices.graphics = i;
        }

        auto canPresent = gpu.getSurfaceSupportKHR(i, surface);
        if (canPresent) {
            evaluator.indices.present = i;
        }

        if (evaluator.indices.good())
            break;

        i++;
    }

    return evaluator;
}

static VkBool32 DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT level,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    switch (level) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            std::cerr << Console::log << "Log:";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            std::cerr << Console::warn << "Warn:";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            std::cerr << Console::error << "Error:";
            break;
        default:
            std::cerr << Console::clear;
            break;
    }

    std::cerr << Console::log << pCallbackData->pMessage;
    std::cerr << Console::clear << std::endl;

    return VK_FALSE;
}

static const std::vector<const char*> kLayers = {"VK_LAYER_KHRONOS_validation"};

static const std::vector<const char*> kExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

std::vector<const char*> getWindowExtensions(SDL_Window* window) {
    SDL_bool result = SDL_TRUE;
    std::vector<const char*> extensions{};
    uint32_t extCount = 0;
    result = SDL_Vulkan_GetInstanceExtensions(window, &extCount, nullptr);
    extensions.resize(extCount);
    result =
        SDL_Vulkan_GetInstanceExtensions(window, &extCount, extensions.data());
    return extensions;
}
}  // namespace helper

bool Renderer_D::init(std::shared_ptr<Window_D> parent) {
    window = parent;
    initInstance();
    initSurface();
    initGpu();
    initDevice();
    initQueue();
    initSwapchain();
    initSwapImages();
    initDepthImage();
    initRenderPass();
    initFramebuffer();
    return true;
}

Renderer_D::~Renderer_D() {
#define SAFE_DESTROY(p, o) \
    if (o)                 \
    (p).destroy(o)
#define SAFE_FREE(p, o) \
    if (o)              \
    p.free(o)
    if (device) {
        for (uint32_t i = 0; i < numOfImage; i++) {
            SAFE_DESTROY(device, framebuffers[i]);
            SAFE_DESTROY(device, swapImages[i]);
        }
        SAFE_DESTROY(device, renderPass);
        SAFE_DESTROY(device, depthBuffer.image);
        SAFE_DESTROY(device, depthBuffer.view);
        SAFE_FREE(device, depthBuffer.memory);
        SAFE_DESTROY(device, swapchain);
        device.destroy();
    }
    if (instance) {
        SAFE_DESTROY(instance, surface);
        instance.destroy();
    }
#undef SAFE_DESTROY
#undef SAFE_FREE
}

bool Renderer_D::findMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask,
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

void Renderer_D::initInstance() {
    vk::Result result;

    auto appCI = vk::ApplicationInfo()
                     .setPNext(nullptr)
                     .setPApplicationName("Vulkan Engine")
                     .setApplicationVersion(0)
                     .setApiVersion(VK_API_VERSION_1_1)
                     .setPEngineName("None")
                     .setEngineVersion(0);

    auto extensions = helper::getWindowExtensions(window->window);
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
            .setPfnUserCallback(helper::DebugCallback);

    auto instCI = vk::InstanceCreateInfo()
                      .setPEnabledLayerNames(helper::kLayers)
                      .setPEnabledExtensionNames(extensions)
                      .setPApplicationInfo(&appCI)
                      .setPNext(&debugCI);

    result = vk::createInstance(&instCI, nullptr, &instance);
    assert(result == vk::Result::eSuccess);
}

void Renderer_D::initSurface() {
    VkSurfaceKHR cSurf;
    SDL_Vulkan_CreateSurface(window->window, instance, &cSurf);
    surface = cSurf;
    assert(surface);
}

void Renderer_D::initGpu() {
    auto availableGPUs = instance.enumeratePhysicalDevices();
    bool found = false;
    for (const auto& e : availableGPUs) {
        auto aux = helper::GpuAux::Query(e, surface);
        if (aux.check(helper::kExtensions)) {
            gpu = e;
            indices = aux.indices;
            format = aux.getFormat();
            extent = aux.getExtent();
            found = true;
            break;
        }
    }
    assert(found);
}

void Renderer_D::initDevice() {
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
            .setPEnabledExtensionNames(helper::kExtensions)
            .setPEnabledLayerNames(helper::kLayers)
            .setPEnabledFeatures(nullptr);

    result = gpu.createDevice(&deviceCI, nullptr, &device);
    assert(result == vk::Result::eSuccess);
}

void Renderer_D::initQueue() {
    queues.graphics = device.getQueue(indices.graphics, 0);
    queues.present = device.getQueue(indices.present, 0);
}

void Renderer_D::initSwapchain() {
    vk::Result result;
    auto aux = helper::GpuAux::Query(gpu, surface);
    result = aux.createSwapchain(device, surface, swapchain);
    assert(result == vk::Result::eSuccess);
}

void Renderer_D::initSwapImages() {
    vk::Result result;

    auto images = device.getSwapchainImagesKHR(swapchain);
    assert(images.size() > 0);
    numOfImage = (uint32_t)images.size();

    auto resRange = vk::ImageSubresourceRange()
                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                        .setLayerCount(1)
                        .setBaseArrayLayer(0)
                        .setLevelCount(1)
                        .setBaseMipLevel(0);

    swapImages = std::make_unique<vk::ImageView[]>(numOfImage);
    for (size_t i = 0; i < numOfImage; i++) {
        vk::ImageViewCreateInfo imageViewCI =
            vk::ImageViewCreateInfo()
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(format.format)
                .setPNext(nullptr)
                .setSubresourceRange(resRange);
        imageViewCI.setImage(images[i]);
        result = device.createImageView(&imageViewCI, nullptr, &swapImages[i]);
        assert(result == vk::Result::eSuccess);
    }
}

void Renderer_D::initDepthImage() {
    vk::Result result;

    auto imageCI =
        vk::ImageCreateInfo()
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
    result = device.createImage(&imageCI, nullptr, &depthBuffer.image);
    assert(result == vk::Result::eSuccess);

    vk::MemoryAllocateInfo memoryAI;
    vk::MemoryRequirements memReq;
    device.getImageMemoryRequirements(depthBuffer.image, &memReq);
    memoryAI.setAllocationSize(memReq.size);
    auto pass = findMemoryType(memReq.memoryTypeBits,
                               vk::MemoryPropertyFlagBits::eDeviceLocal,
                               memoryAI.memoryTypeIndex);
    assert(pass);
    result = device.allocateMemory(&memoryAI, nullptr, &depthBuffer.memory);
    assert(result == vk::Result::eSuccess);

    device.bindImageMemory(depthBuffer.image, depthBuffer.memory, 0);
    auto imageViewCI = vk::ImageViewCreateInfo()
                           .setImage(depthBuffer.image)
                           .setViewType(vk::ImageViewType::e2D)
                           .setFormat(vk::Format::eD16Unorm)
                           .setSubresourceRange(vk::ImageSubresourceRange(
                               vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1))
                           .setPNext(nullptr);
    result = device.createImageView(&imageViewCI, nullptr, &depthBuffer.view);
    assert(result == vk::Result::eSuccess);
}

void Renderer_D::initRenderPass() {
    vk::Result result;

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
    assert(result == vk::Result::eSuccess);
}

void Renderer_D::initFramebuffer() {
    framebuffers = std::make_unique<vk::Framebuffer[]>(numOfImage);

    vk::ImageView attachments[2];
    attachments[1] = depthBuffer.view;

    auto frameBufferCI = vk::FramebufferCreateInfo()
                             .setRenderPass(renderPass)
                             .setAttachmentCount(2)
                             .setPAttachments(attachments)
                             .setWidth(extent.width)
                             .setHeight(extent.height)
                             .setLayers(1);

    for (uint32_t i = 0; i < numOfImage; i++) {
        attachments[0] = swapImages[i];
        auto result =
            device.createFramebuffer(&frameBufferCI, nullptr, &framebuffers[i]);
        assert(result == vk::Result::eSuccess);
    }
}

Renderer::Renderer() {
    initImpl();
}

Renderer::~Renderer() {}

void Renderer::bindWindow(Window& window) {
    auto winImpl = window.shareImpl();
    if (winImpl) {
        getImpl()->init(winImpl);
    }
}
#include "render/device.h"
#include "render/device_d.h"

#include <iostream>
#include <set>

#include <SDL2/SDL_vulkan.h>

#include "public/console.h"
#include "window/window_d.h"

namespace helper {

struct GPUEvaluator {
    GPUEvaluator& querySupport(const vk::PhysicalDevice& gpu, const vk::SurfaceKHR& surface) {
        gpu.getSurfaceCapabilitiesKHR(surface, &capabilities);
        formats = gpu.getSurfaceFormatsKHR(surface);
        presentModes = gpu.getSurfacePresentModesKHR(surface);
        extensions = gpu.enumerateDeviceExtensionProperties();
        return *this;
    }

    vk::SurfaceCapabilitiesKHR capabilities{};
    std::vector<vk::SurfaceFormatKHR> formats{};
    std::vector<vk::PresentModeKHR> presentModes{};
    std::vector<vk::ExtensionProperties> extensions;

    void setSwapchain(vk::SwapchainCreateInfoKHR& info);
};


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

static std::vector<const char*> GetWindowExts(SDL_Window* window) {
    SDL_bool result = SDL_TRUE;
    std::vector<const char*> extensions{};
    uint32_t extCount = 0;
    result = SDL_Vulkan_GetInstanceExtensions(window, &extCount, nullptr);
    extensions.resize(extCount);
    result = SDL_Vulkan_GetInstanceExtensions(window, &extCount, extensions.data());
    return extensions;
}

static const std::vector<const char*> kLayers = {
    "VK_LAYER_KHRONOS_validation"
};

static const std::vector<const char*> kExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

}

Renderer_D::QueueIndices
Renderer_D::acquireIndices(const vk::PhysicalDevice& gpu) const {
    QueueIndices tmpIndices{};

    uint32_t i = 0;
    auto queueFamilies = gpu.getQueueFamilyProperties();
    for (const auto& e : queueFamilies) {
        if (e.queueFlags & vk::QueueFlagBits::eGraphics) {
            tmpIndices.graphics = i;
        }

        auto canPresent = gpu.getSurfaceSupportKHR(i, surface);
        if (canPresent) {
            tmpIndices.present = i;
        }

        if (tmpIndices.good())
            break;

        i++;
    }

    return tmpIndices;
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

    auto extensions = helper::GetWindowExts(window->window);
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

bool Renderer_D::checkGpu(const vk::PhysicalDevice& gpu) const {
    auto tmpIndices = acquireIndices(gpu);
    auto evaluator = helper::GPUEvaluator().querySupport(gpu, surface);

    std::set<std::string> seqExts(
        helper::kExtensions.begin(), helper::kExtensions.end());
    for (const auto& extension : evaluator.extensions) {
        seqExts.erase(extension.extensionName);
    }
    bool extensionsOk = seqExts.empty();

    bool swapchainOk = false;
    if (extensionsOk) {
        swapchainOk = !evaluator.formats.empty()
            && !evaluator.presentModes.empty();
    }

    return tmpIndices.good() && extensionsOk && swapchainOk;
}

void Renderer_D::initGpu() {
    auto availableGPUs = instance.enumeratePhysicalDevices();
    bool found = false;
    for (const auto& e : availableGPUs) {
        if (checkGpu(e)) {
            gpu = e;
            found = true;
            break;
        }
    }
    assert(found);
}

void Renderer_D::initQueueIndices() {
    indices = acquireIndices(gpu);
    assert(indices.good());
}

void Renderer_D::initDevice() {
    vk::Result result;

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
    std::set<uint32_t> queueFamilies = { indices.graphics, indices.present };

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
    auto evaluator = helper::GPUEvaluator().querySupport(gpu, surface);
    
    auto presentMode = evaluator.selectPresentMode();
    auto format = evaluator.selectFormat();
    auto extent = evaluator.selectExtent();
    auto imageCount = evaluator.selectImageCount();
    auto alpha = evaluator.selectAlpha();

    auto swapchainCI =
        vk::SwapchainCreateInfoKHR()
        .setPNext(nullptr)
        .setMinImageCount(imageCount)
        .setImageFormat(format.format)
        .setImageColorSpace(format.colorSpace)
        .setImageExtent(extent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setPreTransform(evaluator.capabilities.currentTransform)
        .setCompositeAlpha(alpha)
        .setPresentMode(presentMode)
        .setClipped(true)
        .setSurface(surface);

    std::vector<uint32_t> swapIndices{ indices.graphics };
    if (indices.graphics != indices.present) {
        swapIndices.push_back(indices.present);
        swapchainCI.setImageSharingMode(vk::SharingMode::eConcurrent);
    } else {
        swapchainCI.setImageSharingMode(vk::SharingMode::eExclusive);
    }
    swapchainCI.setQueueFamilyIndices(swapIndices);

    result = device.createSwapchainKHR(&swapchainCI, nullptr, &swapchain);
    assert(result == vk::Result::eSuccess);
}

void Renderer_D::initSwapImages() {
    vk::Result result;

    auto images = device.getSwapchainImagesKHR(swapchain);
    assert(images.size() > 0);
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

void Renderer_D::initDepthImage() {
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
    std::vector<uint32_t> depthIndices{ indices.graphics };
    if (indices.graphics != indices.present) {
        depthIndices.push_back(indices.present);
        imageCI.setSharingMode(vk::SharingMode::eConcurrent);
    } else {
        imageCI.setSharingMode(vk::SharingMode::eExclusive);
    }
    imageCI.setQueueFamilyIndices(depthIndices);
    result = device.createImage(&imageCI, nullptr, &depthImage.image);
    assert(result == vk::Result::eSuccess);

    vk::MemoryAllocateInfo memoryAI;
    vk::MemoryRequirements memReq;
    device.getImageMemoryRequirements(depthImage.image, &memReq);
    memoryAI.setAllocationSize(memReq.size);
    auto pass = getMemoryType(memReq.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        memoryAI.memoryTypeIndex);
    assert(pass);
    result = device.allocateMemory(&memoryAI, nullptr, &depthImage.memory);
    assert(result == vk::Result::eSuccess);

    device.bindImageMemory(depthImage.image, depthImage.memory, 0);
    auto imageViewCI = vk::ImageViewCreateInfo()
        .setImage(depthImage.image)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(vk::Format::eD16Unorm)
        .setSubresourceRange(vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1))
        .setPNext(nullptr);
    result = device.createImageView(&imageViewCI, nullptr, &depthImage.view);
    assert(result == vk::Result::eSuccess);
}

void Renderer_D::initRenderPass() {
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
            .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal) };

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
        assert(result == vk::Result::eSuccess);
    }
}

Renderer::Renderer() {
    initImpl();
}

Renderer::~Renderer() {

}

void Renderer::bindWindow(Window& window) {
    getImpl()->window = window.shareImpl();
    getImpl()->initContext();
    getImpl()->initDevice();
}

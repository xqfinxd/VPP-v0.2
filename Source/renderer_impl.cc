#include "renderer_impl.h"

#include <iostream>
#include <set>

#include <SDL2/SDL_vulkan.h>

#include "window_impl.h"

namespace VPP {

namespace impl {

const std::vector<const char*> g_Extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static const std::vector<const char*> g_Layers = {
    "VK_LAYER_KHRONOS_validation"
};

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
        queueProperties =
            std::make_unique<vk::QueueFamilyProperties[]>(indexCount);
        gpu.getQueueFamilyProperties(&indexCount, queueProperties.get());

        queuePresents = std::make_unique<uint32_t[]>(indexCount);
        for (uint32_t i = 0; i < indexCount; ++i) {
            queuePresents[i] = gpu.getSurfaceSupportKHR(i, surface);
        }

        return *this;
    }

    vk::SurfaceFormatKHR getFormat() const {
        for (const auto& fmt : formats) {
            if (fmt.format == vk::Format::eB8G8R8A8Srgb &&
                fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return fmt;
            }
        }

        return formats[0];
    }

    vk::PresentModeKHR getPresentMode() const {
        for (const auto& mode : presentModes) {
            if (mode == vk::PresentModeKHR::eMailbox) {
                return mode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D getExtent() const {
        if (capabilities.currentExtent.width != UINT32_MAX &&
            capabilities.currentExtent.height != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            return capabilities.minImageExtent;
        }
    }

    uint32_t getMinImageCount() const {
        return capabilities.minImageCount;
    }

    QueueIndices getIndices() const {
        QueueIndices indices{};
        for (uint32_t i = 0; i < indexCount; ++i) {
            if (queueProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphics = i;
            }

            if (queuePresents[i] == VK_TRUE) {
                indices.present = i;
            }

            if (indices.good()) {
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

        auto indices = getIndices();

        return extensionsOk && surfaceOk && indices.good();
    }

    vk::Result createSwapchain(const vk::Device&     device,
                               const vk::SurfaceKHR& surface,
                               vk::SwapchainKHR&     swapchain) const {
        auto info = vk::SwapchainCreateInfoKHR()
                        .setImageArrayLayers(1)
                        .setClipped(true)
                        .setSurface(surface);

        auto presentMode = getPresentMode();
        auto format = getFormat();
        auto extent = getExtent();
        auto imageCount = getMinImageCount();
        auto indices = getIndices();

        info.setMinImageCount(imageCount)
            .setImageFormat(format.format)
            .setImageColorSpace(format.colorSpace)
            .setImageExtent(extent)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setPreTransform(capabilities.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(presentMode);

        auto swapIndices = indices.pack();

        if (swapIndices.size() > 1) {
            info.setImageSharingMode(vk::SharingMode::eConcurrent);
        } else {
            info.setImageSharingMode(vk::SharingMode::eExclusive);
        }
        info.setQueueFamilyIndices(swapIndices);

        return device.createSwapchainKHR(&info, nullptr, &swapchain);
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

Renderer::Renderer() {

}

Renderer::~Renderer() {
#define SAFE_DESTROY(p, o) \
    if (o)                 \
    (p).destroy(o)
#define SAFE_FREE(p, o) \
    if (o)              \
    p.free(o)
    if (device) {
        for (uint32_t i = 0; i < context.imageCount; i++) {
            SAFE_DESTROY(device, context.frames[i].framebuffer);
            SAFE_DESTROY(device, context.frames[i].colorbufferView);
        }
        SAFE_DESTROY(device, context.renderPass);
        SAFE_DESTROY(device, context.depthbuffer);
        SAFE_DESTROY(device, context.depthbufferView);
        SAFE_FREE(device, context.depthbufferMemory);
        SAFE_DESTROY(device, context.swapchain);
        device.destroy();
    }
    if (instance) {
        SAFE_DESTROY(instance, surface);
        instance.destroy();
    }
#undef SAFE_DESTROY
#undef SAFE_FREE
}

void Renderer::Setup() {
    vk::Result result;

    {  // instance
        auto appCI = vk::ApplicationInfo()
                         .setPNext(nullptr)
                         .setPApplicationName("Vulkan Engine")
                         .setApplicationVersion(0)
                         .setApiVersion(VK_API_VERSION_1_1)
                         .setPEngineName("None")
                         .setEngineVersion(0);

        auto& wnd = Window::getMe();
        auto  extensions = GetWindowExtensions(wnd.window);
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

        result = vk::createInstance(&instCI, nullptr, &instance);
        assert(result == vk::Result::eSuccess);
    }
    {  // surface
        VkSurfaceKHR cSurf;
        auto&        wnd = Window::getMe();
        SDL_Vulkan_CreateSurface(wnd.window, instance, &cSurf);
        surface = cSurf;
        assert(surface);
    }
    { // physical device
        auto availableGPUs = instance.enumeratePhysicalDevices();
        bool found = false;
        RenderSupport support{};
        for (const auto& e : availableGPUs) {
            support.Query(e, surface);
            if (support.Check(g_Extensions)) {
                gpu = e;
                found = true;
                break;
            }
        }
        assert(found);
        auto extent = support.getExtent();
        context.width = extent.width;
        context.height = extent.height;
        context.surfaceFormat = support.getFormat();
        context.presentMode = support.getPresentMode();
        context.minImageCount = support.getMinImageCount();

        indices = support.getIndices();
    }
    { // device
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
                .setPEnabledExtensionNames(g_Extensions)
                .setPEnabledLayerNames(g_Layers)
                .setPEnabledFeatures(nullptr);

        result = gpu.createDevice(&deviceCI, nullptr, &device);
        assert(result == vk::Result::eSuccess);
    }
    { // queue
        queues.graphics = device.getQueue(indices.graphics, 0);
        queues.present = device.getQueue(indices.present, 0);
    }
}

void Renderer::SetupContext() {
    vk::Result result;

    { // swapchain
        auto info =
            vk::SwapchainCreateInfoKHR()
                .setImageArrayLayers(1)
                .setClipped(true)
                .setSurface(surface)
                .setMinImageCount(context.minImageCount)
                .setImageFormat(context.surfaceFormat.format)
                .setImageColorSpace(context.surfaceFormat.colorSpace)
                .setImageExtent(vk::Extent2D(context.width, context.height))
                .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                .setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
                .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                .setPresentMode(context.presentMode);

        auto indicesData = indices.pack();

        if (indicesData.size() > 1) {
            info.setImageSharingMode(vk::SharingMode::eConcurrent);
        } else {
            info.setImageSharingMode(vk::SharingMode::eExclusive);
        }
        info.setQueueFamilyIndices(indicesData);

        result = device.createSwapchainKHR(&info, nullptr, &context.swapchain);
        assert(result == vk::Result::eSuccess);
    }
    { // depth buffer
        auto imageCI =
            vk::ImageCreateInfo()
                .setFormat(vk::Format::eD16Unorm)
                .setImageType(vk::ImageType::e2D)
                .setExtent(vk::Extent3D(context.width, context.height, 1u))
                .setMipLevels(1)
                .setArrayLayers(1)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setTiling(vk::ImageTiling::eOptimal)
                .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
                .setInitialLayout(vk::ImageLayout::eUndefined);
        auto indicesData = indices.pack();
        if (indices.graphics != indices.present) {
            indicesData.push_back(indices.present);
            imageCI.setSharingMode(vk::SharingMode::eConcurrent);
        } else {
            imageCI.setSharingMode(vk::SharingMode::eExclusive);
        }
        imageCI.setQueueFamilyIndices(indicesData);
        result = device.createImage(&imageCI, nullptr, &context.depthbuffer);
        assert(result == vk::Result::eSuccess);

        vk::MemoryAllocateInfo memoryAI;
        vk::MemoryRequirements memReq;
        device.getImageMemoryRequirements(context.depthbuffer, &memReq);
        memoryAI.setAllocationSize(memReq.size);
        auto pass = findMemoryType(memReq.memoryTypeBits,
                                   vk::MemoryPropertyFlagBits::eDeviceLocal,
                                   memoryAI.memoryTypeIndex);
        assert(pass);
        result = device.allocateMemory(&memoryAI, nullptr,
                                       &context.depthbufferMemory);
        assert(result == vk::Result::eSuccess);

        device.bindImageMemory(context.depthbuffer, context.depthbufferMemory,
                               0);
        auto imageViewCI = vk::ImageViewCreateInfo()
                               .setImage(context.depthbuffer)
                               .setViewType(vk::ImageViewType::e2D)
                               .setFormat(vk::Format::eD16Unorm)
                               .setSubresourceRange(vk::ImageSubresourceRange(
                                   vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1))
                               .setPNext(nullptr);
        result = device.createImageView(&imageViewCI, nullptr,
                                        &context.depthbufferView);
        assert(result == vk::Result::eSuccess);
    }
    { // render pass
        std::vector<vk::AttachmentDescription> attachments = {
            vk::AttachmentDescription()
                .setFormat(context.surfaceFormat.format)
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
                .setFinalLayout(
                    vk::ImageLayout::eDepthStencilAttachmentOptimal)};

        auto colorReference =
            vk::AttachmentReference().setAttachment(0).setLayout(
                vk::ImageLayout::eColorAttachmentOptimal);

        auto depthReference =
            vk::AttachmentReference().setAttachment(1).setLayout(
                vk::ImageLayout::eDepthStencilAttachmentOptimal);

        auto subpass =
            vk::SubpassDescription()
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

        result = device.createRenderPass(&renderPassCI, nullptr,
                                         &context.renderPass);
        assert(result == vk::Result::eSuccess);
    }
    {  // clear
        context.clearEnable = true;
        context.clearValue.setColor({0.0f, 0.0f, 0.0f, 1.0f});
        context.imageCount = 0;
        context.frameIndex = 0;
        context.semaphoreIndex = 0;
    }
    { // frame
        auto images = device.getSwapchainImagesKHR(context.swapchain);
        assert(images.size() > 0);
        context.imageCount = (uint32_t)images.size();

        context.frames = std::make_unique<RenderFrame[]>(context.imageCount);
        for (uint32_t i = 0; i < context.imageCount; ++i) {
            context.frames[i].colorbuffer = images[i];
            createFrame(context.frames[i]);
        }

        context.semaphores = std::make_unique<FrameSemaphores[]>(context.imageCount);
        for (uint32_t i = 0; i < context.imageCount; ++i) {
            createSemaphore(context.semaphores[i]);
        }
    }

}

void Renderer::createFrame(RenderFrame& frame) {
    vk::Result result;

    { // color buffer view
        auto resRange = vk::ImageSubresourceRange()
                            .setAspectMask(vk::ImageAspectFlagBits::eColor)
                            .setLayerCount(1)
                            .setBaseArrayLayer(0)
                            .setLevelCount(1)
                            .setBaseMipLevel(0);

        vk::ImageViewCreateInfo imageViewCI =
            vk::ImageViewCreateInfo()
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(context.surfaceFormat.format)
                .setPNext(nullptr)
                .setSubresourceRange(resRange);
        imageViewCI.setImage(frame.colorbuffer);
        result = device.createImageView(&imageViewCI, nullptr,
                                        &frame.colorbufferView);
        assert(result == vk::Result::eSuccess);
    }
    { // frame buffer
        vk::ImageView attachments[2];
        attachments[1] = context.depthbufferView;

        auto frameBufferCI = vk::FramebufferCreateInfo()
                                 .setRenderPass(context.renderPass)
                                 .setAttachmentCount(2)
                                 .setPAttachments(attachments)
                                 .setWidth(context.width)
                                 .setHeight(context.height)
                                 .setLayers(1);

        attachments[0] = frame.colorbufferView;
        auto result = device.createFramebuffer(&frameBufferCI, nullptr,
                                               &frame.framebuffer);
        assert(result == vk::Result::eSuccess);
    }
    { // command pool
        auto info = vk::CommandPoolCreateInfo()
            .setQueueFamilyIndex(indices.graphics);
        result = device.createCommandPool(&info, nullptr, &frame.commandPool);
        assert(result == vk::Result::eSuccess);
    }
    {  // command buffer
        auto info = vk::CommandBufferAllocateInfo()
            .setCommandPool(frame.commandPool)
                        .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(1);
        
        result = device.allocateCommandBuffers(&info, &frame.commandBuffer);
        assert(result == vk::Result::eSuccess);
    }
    { // fence
        auto const fenceCI =
            vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
        result = device.createFence(&fenceCI, nullptr, &frame.fence);
        assert(result == vk::Result::eSuccess);
    }
}
void Renderer::createSemaphore(FrameSemaphores& semaphore) {
    vk::Result result;

    auto       semaphoreCreateInfo = vk::SemaphoreCreateInfo();
    result = device.createSemaphore(&semaphoreCreateInfo, nullptr,
                                    &semaphore.imageAcquired);
    assert(result == vk::Result::eSuccess);

    result = device.createSemaphore(&semaphoreCreateInfo, nullptr,
                                    &semaphore.renderComplete);
    assert(result == vk::Result::eSuccess);
}

bool Renderer::findMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask,
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

}  // namespace impl

}



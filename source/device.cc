#include "device.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cassert>
#include <iostream>
#include <vector>
#include <set>

extern GLFWwindow* GetWindow();

static const std::vector<const char*> kValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
};

static const std::vector<const char*> kDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VkBool32 DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

static void SetDebugMessengerInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
}

static std::vector<const char*> GetRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

void RenderDevice::Init() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickGpu();
    CreateDeviceAndQueue();
    CreateSwapchain();
    CreateImageViews();
}

void RenderDevice::Quit() {
    for (uint32_t i = 0; i < color_image_count_; i++) {
        vkDestroyImageView(device_, color_image_views_[i], nullptr);
    }
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    vkDestroyDevice(device_, nullptr);
    DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);
}

void RenderDevice::CreateInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "vklua";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "vk";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
    createInfo.ppEnabledLayerNames = kValidationLayers.data();
    SetDebugMessengerInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    
    assert(vkCreateInstance(&createInfo, nullptr, &instance_) == VK_SUCCESS);
}

void RenderDevice::SetupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    SetDebugMessengerInfo(createInfo);

    assert(CreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debug_messenger_) == VK_SUCCESS);
}

void RenderDevice::CreateSurface() {
    auto window = GetWindow();
    assert(window);
    assert(glfwCreateWindowSurface(instance_, window, nullptr, &surface_) == VK_SUCCESS);
}

void RenderDevice::PickGpu() {
    gpu_ = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (IsDeviceSuitable(device)) {
            gpu_ = device;
            break;
        }
    }

    assert(gpu_ != VK_NULL_HANDLE);
}

void RenderDevice::CreateDeviceAndQueue() {
    QueueFamilyIndices indices = FindQueueFamilies(gpu_);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphics, indices.present };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

    createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
    createInfo.ppEnabledLayerNames = kValidationLayers.data();

    assert(vkCreateDevice(gpu_, &createInfo, nullptr, &device_) == VK_SUCCESS);

    vkGetDeviceQueue(device_, indices.graphics, 0, &graphics_queue_);
    vkGetDeviceQueue(device_, indices.present, 0, &present_queue_);
}

void RenderDevice::CreateSwapchain() {
    SwapchainSupport swapChainSupport = QuerySwapChainSupport(gpu_);

    VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSurfacePresentMode(swapChainSupport.present_modes);
    VkExtent2D extent = ChooseSurfaceExtent(swapChainSupport.capabilities);

    color_image_count_ = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && color_image_count_ > swapChainSupport.capabilities.maxImageCount) {
        color_image_count_ = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;

    createInfo.minImageCount = color_image_count_;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = FindQueueFamilies(gpu_);
    uint32_t queueFamilyIndices[] = { indices.graphics, indices.present };

    if (indices.graphics != indices.present) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    assert(vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_) == VK_SUCCESS);

    vkGetSwapchainImagesKHR(device_, swapchain_, &color_image_count_, nullptr);
    color_images_ = std::make_unique<VkImage[]>(color_image_count_);
    vkGetSwapchainImagesKHR(device_, swapchain_, &color_image_count_, color_images_.get());

    color_image_format_ = surfaceFormat.format;
    color_image_extent_ = extent;
}

void RenderDevice::CreateImageViews() {
    color_image_views_ = std::make_unique<VkImageView[]>(color_image_count_);

    for (size_t i = 0; i < color_image_count_; i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = color_images_[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = color_image_format_;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        assert(vkCreateImageView(device_, &createInfo, nullptr, &color_image_views_[i]) == VK_SUCCESS);
    }
}

bool RenderDevice::IsDeviceSuitable(const VkPhysicalDevice& gpu) const {
    QueueFamilyIndices indices = FindQueueFamilies(gpu);

    bool extensionsSupported = CheckDeviceExtensionSupport(gpu);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapchainSupport swapChainSupport = QuerySwapChainSupport(gpu);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.present_modes.empty();
    }

    return indices && extensionsSupported && swapChainAdequate;
}

bool RenderDevice::CheckDeviceExtensionSupport(const VkPhysicalDevice& gpu) const {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(kDeviceExtensions.begin(), kDeviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices RenderDevice::FindQueueFamilies(const VkPhysicalDevice& gpu) const {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface_, &presentSupport);

        if (presentSupport) {
            indices.present = i;
        }

        if (indices) {
            break;
        }

        i++;
    }

    return indices;
}

SwapchainSupport RenderDevice::QuerySwapChainSupport(const VkPhysicalDevice& gpu) const {
    SwapchainSupport details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface_, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface_, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface_, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface_, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.present_modes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface_, &presentModeCount, details.present_modes.data());
    }

    return details;
}

VkSurfaceFormatKHR RenderDevice::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const {
    for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
            && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return formats[0];
}

VkPresentModeKHR RenderDevice::ChooseSurfacePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const {
    for (const auto& availablePresentMode : presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D RenderDevice::ChooseSurfaceExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(GetWindow(), &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = glm::clamp(actualExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        actualExtent.height = glm::clamp(actualExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

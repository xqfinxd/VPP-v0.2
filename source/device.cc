#include "device.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cassert>
#include <iostream>
#include <vector>
#include <set>
#include "window.h"

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

QueueFamilyIndices::QueueFamilyIndices(VkPhysicalDevice gpu, VkSurfaceKHR surface) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &presentSupport);

        if (presentSupport) {
            present = i;
        }

        if (valid()) {
            break;
        }

        i++;
    }
}

SurfaceSupport::SurfaceSupport(VkPhysicalDevice gpu, VkSurfaceKHR surface) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        present_modes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, present_modes.data());
    }
}

VkSurfaceFormatKHR SurfaceSupport::SelectFormat() const {
    for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
            && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return formats[0];
}

VkPresentModeKHR SurfaceSupport::SelectPresentMode() const {
    for (const auto& availablePresentMode : present_modes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SurfaceSupport::SelectExtent() const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = MainWindow::Get()->GetSurfaceExtent();

        actualExtent.width = glm::clamp(actualExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        actualExtent.height = glm::clamp(actualExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

uint32_t SurfaceSupport::SelectImageCount() const {
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }
    return imageCount;
}

Renderer::Renderer() {
    CreateInstance();
    CreateSurface();
    SelectGpu();
    CreateDeviceAndQueue();
}

Renderer::~Renderer() {
    vkDestroyDevice(device_, nullptr);
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);
}

void Renderer::CreateInstance() {
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

    auto extensions = MainWindow::Get()->GetExtensions();
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
    createInfo.ppEnabledLayerNames = kValidationLayers.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = DebugCallback;

    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    
    auto res = vkCreateInstance(&createInfo, nullptr, &instance_);
    assert(res == VK_SUCCESS);
}

void Renderer::CreateSurface() {
    surface_ = MainWindow::Get()->GetSurface(instance_);
}

void Renderer::SelectGpu() {
    gpu_ = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);

    assert(deviceCount);

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (CheckPhysicalDeviceSupport(device)) {
            gpu_ = device;
            break;
        }
    }

    assert(gpu_ != VK_NULL_HANDLE);
}

void Renderer::CreateDeviceAndQueue() {
    QueueFamilyIndices indices(gpu_, surface_);

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

    auto res = vkCreateDevice(gpu_, &createInfo, nullptr, &device_);
    assert(res == VK_SUCCESS);

    vkGetDeviceQueue(device_, indices.graphics, 0, &graphics_queue_);
    vkGetDeviceQueue(device_, indices.present, 0, &present_queue_);
}

bool Renderer::FindMemoryType(uint32_t& index,
    uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(gpu_, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            index = i;
            return true;
        }
    }

    return false;
}

bool Renderer::CheckPhysicalDeviceSupport(const VkPhysicalDevice& gpu) const {
    QueueFamilyIndices indices(gpu, surface_);

    bool extensionsSupported = CheckDeviceExtensionSupport(gpu);

    bool swapchainAdequate = false;
    if (extensionsSupported) {
        SurfaceSupport surfSupport(gpu, surface_);
        swapchainAdequate = !surfSupport.formats.empty() && !surfSupport.present_modes.empty();
    }

    return indices && extensionsSupported && swapchainAdequate;
}

bool Renderer::CheckDeviceExtensionSupport(const VkPhysicalDevice& gpu) const {
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

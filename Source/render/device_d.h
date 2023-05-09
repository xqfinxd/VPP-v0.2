#pragma once

#include <vulkan/vulkan.hpp>

struct Window_D;

struct RenderContext {
    vk::Instance    instance{};
};

struct RenderDevice {
    vk::SurfaceKHR     surface{};
    vk::PhysicalDevice physicalDevice{};
    uint32_t           indexOfGraphics{ UINT32_MAX };
    uint32_t           indexOfPresent{ UINT32_MAX };
    vk::SurfaceFormatKHR surfaceFormat{};
    vk::PresentModeKHR   presentMode{};
    vk::PresentModeKHR   resizePresentMode{};
    vk::Device         device{};
    vk::Queue          queueOfGraphics{};
    vk::Queue          queueOfPresent{};
    vk::CommandPool    commandPool{};
};

struct Swapchain {
    vk::SurfaceCapabilitiesKHR   capabilities{};
    vk::Extent2D                 extent{};
    vk::SwapchainKHR           swapchain{};
    std::vector<vk::ImageView> imageViews{};
};

struct RenderPass {
    vk::RenderPass                  renderPass{};
    std::vector<vk::Framebuffer>    framebuffers{};
    std::vector<vk::CommandBuffer>  commandBuffers{};
};

struct Renderer_D {
    void initContext();
    void initDevice();
    void initSwapchain();
    void initRenderPass();

    std::shared_ptr<Window_D> window;
    RenderContext   context;
    RenderDevice    gpu;
    Swapchain       swapchain;
    RenderPass      renderPass;

    const std::vector<const char*> kValidationLayers {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> kDeviceExtensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};
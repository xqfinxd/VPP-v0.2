#pragma once

#include <vulkan/vulkan.hpp>

struct Window_D;

struct Renderer_D {
    struct QueueIndices {
        uint32_t graphics = UINT32_MAX;
        uint32_t present = UINT32_MAX;

        bool good() const {
            return graphics != UINT32_MAX
                && present != UINT32_MAX;
        }
    };

    struct DeviceQueues {
        vk::Queue graphics{};
        vk::Queue present{};
    };

    struct DepthBuffer {
        vk::Image image{};
        vk::ImageView view{};
        vk::DeviceMemory memory{};
    };

    std::shared_ptr<Window_D>           window{ nullptr };

    vk::Instance                        instance{};
    vk::PhysicalDevice                  gpu{};
    vk::SurfaceKHR                      surface{};
    QueueIndices                        indices{};
    vk::Device                          device{};
    DeviceQueues                        queues{};
    vk::SwapchainKHR                    swapchain{};
    uint32_t                            numOfImage{};
    std::unique_ptr<vk::ImageView[]>    swapImages{};
    DepthBuffer                         depthBuffer{};
    vk::RenderPass                      renderPass{};
    std::unique_ptr<vk::Framebuffer[]>  framebuffers{};
    std::unique_ptr<vk::Semaphore[]>    imageAcquired{};
    std::unique_ptr<vk::Semaphore[]>    drawComplete{};
    std::vector<vk::Fence>              inFlight{};

private:
    QueueIndices acquireIndices(const vk::PhysicalDevice& gpu) const;

    void initInstance();
    void initSurface();
    bool checkGpu(const vk::PhysicalDevice& gpu) const;
    void initGpu();
    void initQueueIndices();
    void initDevice();
    void initQueue();
    void initSwapchain();
    void initSwapImages();
    void initDepthImage();
    void initRenderPass();
    void initFramebuffer();
};

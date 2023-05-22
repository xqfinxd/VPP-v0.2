#pragma once

#include <vulkan/vulkan.hpp>

namespace VPP {

namespace impl {

struct QueueIndices {
    uint32_t graphics = UINT32_MAX;
    uint32_t present = UINT32_MAX;

    bool good() const {
        return graphics != UINT32_MAX && present != UINT32_MAX;
    }

    std::vector<uint32_t> pack() const {
        std::vector<uint32_t> indices{};
        indices.push_back(graphics);
        if (graphics != present) {
            indices.push_back(present);
        }
        return indices;
    }
};

struct DeviceQueues {
    vk::Queue graphics{};
    vk::Queue present{};
};

struct DepthBuffer {
    vk::Image        image{};
    vk::ImageView    view{};
    vk::DeviceMemory memory{};
};

struct RenderFrame {
    vk::CommandPool   commandPool{};
    vk::CommandBuffer commandBuffer{};
    vk::Fence         fence{};
    vk::Image         colorbuffer{};
    vk::ImageView     colorbufferView{};
    vk::Framebuffer   framebuffer{};
};

struct FrameSemaphores {
    vk::Semaphore imageAcquired;
    vk::Semaphore renderComplete;
};

struct RenderContext {
    //  set when surface created
    uint32_t             width{};
    uint32_t             height{};
    vk::SurfaceFormatKHR surfaceFormat{};
    vk::PresentModeKHR   presentMode{};
    uint32_t             minImageCount;

    // set after device created
    vk::SwapchainKHR     swapchain{};
    vk::Image            depthbuffer;
    vk::ImageView        depthbufferView;
    vk::DeviceMemory     depthbufferMemory;
    vk::RenderPass       renderPass{};

    bool           clearEnable{};
    vk::ClearValue clearValue{};

    uint32_t imageCount{};
    uint32_t frameIndex{};
    uint32_t semaphoreIndex{};

    std::unique_ptr<RenderFrame[]>     frames{};
    std::unique_ptr<FrameSemaphores[]> semaphores{};
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    void Setup();
    void SetupContext();
    bool findMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask,
                        uint32_t& typeIndex) const;
    
private:
    vk::Instance       instance{};
    vk::SurfaceKHR     surface{};
    vk::PhysicalDevice gpu{};
    vk::Device         device{};
    QueueIndices       indices{};
    DeviceQueues       queues{};

    RenderContext context{};

    void createFrame(RenderFrame& frame);
    void createSemaphore(FrameSemaphores& semaphore);
};

}  // namespace impl

}  // namespace VPP

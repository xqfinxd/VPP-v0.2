#pragma once

#include <vulkan/vulkan.hpp>

struct Window_D;

namespace helper {
struct QueueIndices {
    uint32_t graphics = UINT32_MAX;
    uint32_t present = UINT32_MAX;

    bool good() const {
        return graphics != UINT32_MAX && present != UINT32_MAX;
    }

    std::vector<uint32_t> pack() const;
};

struct GpuAux {
    static GpuAux Query(const vk::PhysicalDevice& gpu,
                        const vk::SurfaceKHR& surface);

    vk::SurfaceCapabilitiesKHR capabilities{};
    std::vector<vk::SurfaceFormatKHR> formats{};
    std::vector<vk::PresentModeKHR> presentModes{};
    std::vector<vk::ExtensionProperties> extensions{};
    QueueIndices indices{};

    vk::SurfaceFormatKHR getFormat() const;
    vk::PresentModeKHR getPresentMode() const;
    vk::Extent2D getExtent() const;
    uint32_t getNumOfImage() const;
    vk::CompositeAlphaFlagBitsKHR getAlpha() const;

    bool check(const std::vector<const char*>& extensions) const;
    vk::Result createSwapchain(const vk::Device& device,
                               const vk::SurfaceKHR& surface,
                               vk::SwapchainKHR& swapchain) const;
};
}  // namespace helper

struct Renderer_D {
    bool init(std::shared_ptr<Window_D> parent);
    ~Renderer_D();

    struct DeviceQueues {
        vk::Queue graphics{};
        vk::Queue present{};
    };

    struct DepthBuffer {
        vk::Image image{};
        vk::ImageView view{};
        vk::DeviceMemory memory{};
    };
    std::shared_ptr<Window_D> window{};
    vk::Instance instance{};
    vk::SurfaceKHR surface{};
    vk::PhysicalDevice gpu{};
    helper::QueueIndices indices{};
    vk::SurfaceFormatKHR format{};
    vk::Extent2D extent{};
    vk::Device device{};
    DeviceQueues queues{};
    vk::SwapchainKHR swapchain{};
    uint32_t numOfImage{};
    std::unique_ptr<vk::ImageView[]> swapImages{};
    DepthBuffer depthBuffer{};
    vk::RenderPass renderPass{};
    std::unique_ptr<vk::Framebuffer[]> framebuffers{};

    std::unique_ptr<vk::Semaphore[]> imageAcquired{};
    std::unique_ptr<vk::Semaphore[]> drawComplete{};
    std::vector<vk::Fence> inFlight{};

    bool findMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask,
                        uint32_t& typeIndex) const;

private:
    void initInstance();
    void initSurface();
    void initGpu();
    void initDevice();
    void initQueue();
    void initSwapchain();
    void initSwapImages();
    void initDepthImage();
    void initRenderPass();
    void initFramebuffer();
};

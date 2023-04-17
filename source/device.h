#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <set>

#include <vulkan/vulkan.hpp>

#include "utility.h"

using LayerType = const char*;
using ExtensionType = const char*;

struct QueueIndices {
    QueueIndices() = default;
    QueueIndices(const vk::PhysicalDevice& gpu, const vk::SurfaceKHR& surface);

    static constexpr uint32_t INVALID = UINT32_MAX;
    uint32_t graphics = INVALID;
    uint32_t present = INVALID;

    bool valid() const {
        return graphics != INVALID && present != INVALID;
    }

    operator bool() {
        return valid();
    }
};

struct SurfaceSupport {
    SurfaceSupport() = default;
    SurfaceSupport(const vk::PhysicalDevice& gpu, const vk::SurfaceKHR& surface);

    vk::SurfaceCapabilitiesKHR capabilities{};
    std::vector<vk::SurfaceFormatKHR> formats{};
    std::vector<vk::PresentModeKHR> presentModes{};

    vk::SurfaceFormatKHR selectFormat() const;
    vk::PresentModeKHR selectPresentMode() const;
    vk::Extent2D selectExtent() const;
    uint32_t selectImageCount() const;
    vk::CompositeAlphaFlagBitsKHR selectAlpha() const;
};

struct SwapImage {
    vk::Image image;
    vk::ImageView view;
};

struct DepthImage {
    vk::Image image;
    vk::ImageView view;
    vk::DeviceMemory memory;
};

struct RenderSyncObj {
    vk::Fence fences;
    vk::Semaphore imageAcquired;
    vk::Semaphore drawComplete;
};

struct DeviceQueues {
    vk::Queue graphics;
    vk::Queue present;
};

class Renderer
    : public Singleton<Renderer> {
public:
    Renderer();
    ~Renderer();

    void initInstance();
    void initSurface();
    bool checkDeviceExtension(const vk::PhysicalDevice& candidateGpu,
        const std::vector<ExtensionType>& exts) const;
    bool checkPhysicalDevice(const vk::PhysicalDevice& gpu) const;
    void initGpu();
    void initQueueIndices();
    void initDevice();
    void initQueue();
    void initSwapchain();
    void initSwapImages();
    void initDepthImage();
    void initRenderPass();
    void initFramebuffer();

    bool getMemoryType(
        uint32_t memType,
        vk::MemoryPropertyFlags mask,
        uint32_t& typeIndex
    ) const;

private:
    bool prepared = false;

    vk::Instance instance;
    vk::PhysicalDevice gpu;
    vk::SurfaceKHR surface;
    QueueIndices indices;
    vk::Device device;
    DeviceQueues queues;
    vk::SwapchainKHR swapchain;
    uint32_t swapImageCount;
    std::unique_ptr<SwapImage[]> swapImages{};
    DepthImage depthImage{};
    vk::RenderPass renderPass;
    std::unique_ptr<vk::Framebuffer[]> framebuffers;

    uint32_t frameIndex = 0;
    uint32_t currentBuffer = 0;

    std::vector<RenderSyncObj> syncObjs{};
};

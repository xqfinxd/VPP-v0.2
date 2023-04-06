#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include <vulkan/vulkan.h>

struct QueueFamilyIndices {
	static constexpr uint32_t INVALID = UINT32_MAX;
	uint32_t graphics = INVALID;
	uint32_t present = INVALID;

	operator bool() {
		return graphics != INVALID && present != INVALID;
	}
};

struct SwapchainSupport {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

class RenderDevice {
public:
	void Init();
	void Quit();

protected:
	void CreateInstance();
	void SetupDebugMessenger();
	void CreateSurface();
	void PickGpu();
	void CreateDeviceAndQueue();
    void CreateSwapchain();
	void CreateImageViews();

protected: // tool
	bool IsDeviceSuitable(const VkPhysicalDevice& gpu) const;
	bool CheckDeviceExtensionSupport(const VkPhysicalDevice& gpu) const;
	QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& gpu) const;
	SwapchainSupport QuerySwapChainSupport(const VkPhysicalDevice& gpu) const;
    VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
    VkPresentModeKHR ChooseSurfacePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
    VkExtent2D ChooseSurfaceExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

private:
	VkInstance instance_;
	VkDebugUtilsMessengerEXT debug_messenger_;
	VkSurfaceKHR surface_;
	VkPhysicalDevice gpu_;
	VkDevice device_;
	VkQueue graphics_queue_;
	VkQueue present_queue_;
    VkSwapchainKHR swapchain_;
    std::unique_ptr<VkImage[]> color_images_;
	std::unique_ptr<VkImageView[]> color_image_views_;
    uint32_t color_image_count_;
    VkFormat color_image_format_;
    VkExtent2D color_image_extent_;
};

inline RenderDevice& GetDevice() {
	static RenderDevice kRenderDevice{};
	return kRenderDevice;
}
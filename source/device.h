#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include <vulkan/vulkan.h>

struct QueueFamilyIndices {
	QueueFamilyIndices(VkPhysicalDevice gpu, VkSurfaceKHR surface);

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
	SurfaceSupport(VkPhysicalDevice gpu, VkSurfaceKHR surface);

	VkSurfaceCapabilitiesKHR capabilities{};
	std::vector<VkSurfaceFormatKHR> formats{};
	std::vector<VkPresentModeKHR> present_modes{};

	VkSurfaceFormatKHR SelectFormat() const;
	VkPresentModeKHR SelectPresentMode() const;
	VkExtent2D SelectExtent() const;
	uint32_t SelectImageCount() const;
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
	bool CheckPhysicalDeviceSupport(const VkPhysicalDevice& gpu) const;
	bool CheckDeviceExtensionSupport(const VkPhysicalDevice& gpu) const;
	bool FindMemoryType(uint32_t& index,
		uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	void CreateImage(VkImage& image, VkDeviceMemory& memory,
		VkImageCreateInfo& info, VkMemoryPropertyFlags properties);

private:
	VkInstance instance_{};
	VkDebugUtilsMessengerEXT debug_messenger_{};
	VkSurfaceKHR surface_{};
	VkPhysicalDevice gpu_{};
	VkDevice device_{};
	VkQueue graphics_queue_{};
	VkQueue present_queue_{};
    VkSwapchainKHR swapchain_{};
    uint32_t swap_image_count_{};
    std::unique_ptr<VkImage[]> swap_images_{};
	std::unique_ptr<VkImageView[]> swap_image_views_{};
    VkFormat swap_image_format_{};
    VkExtent2D swap_image_extent_{};
};

inline RenderDevice& GetDevice() {
	static RenderDevice kRenderDevice{};
	return kRenderDevice;
}
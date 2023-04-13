#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include <vulkan/vulkan.h>

#include "utility.h"

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

class Renderer
	: public Singleton<Renderer> {
public:
	Renderer();
	~Renderer();

	bool FindMemoryType(uint32_t& index,
		uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

protected:
	void CreateInstance();
	void CreateSurface();
	void SelectGpu();
	void CreateDeviceAndQueue();

protected: // tool
	bool CheckPhysicalDeviceSupport(const VkPhysicalDevice& gpu) const;
	bool CheckDeviceExtensionSupport(const VkPhysicalDevice& gpu) const;

private:
	VkInstance instance_{};
	VkSurfaceKHR surface_{};
	VkPhysicalDevice gpu_{};
	VkDevice device_{};
	VkQueue graphics_queue_{};
	VkQueue present_queue_{};
};

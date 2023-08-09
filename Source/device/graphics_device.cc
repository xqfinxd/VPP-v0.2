#pragma once

#include "graphics_device.h"

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>
#include <map>

namespace VPP {

GraphicsDevice::GraphicsDevice(SDL_Window* window) {
  assert(window);
  assert(SDL_GetWindowFlags(window) & SDL_WINDOW_VULKAN);
  window_ = window;

  {
    auto result = InitInstance();
    assert(result == vk::Result::eSuccess);
  }
  {
    auto result = InitSurface();
    assert(result);
  }
  {
    auto result = InitPhysicalDevice();
    assert(result);
  }
  {
    auto result = InitDevice();
    assert(result == vk::Result::eSuccess);
  }
}

GraphicsDevice::~GraphicsDevice() {
  if (instance_ && surface_) instance_.destroy(surface_);
}

std::vector<const char*> GraphicsDevice::GetInstanceExtensions() const {
  std::vector<const char*> extensions;
  uint32_t extensionCount = 0;
  SDL_Vulkan_GetInstanceExtensions(window_, &extensionCount, nullptr);
  assert(extensionCount);
  extensions.resize(extensionCount);
  SDL_Vulkan_GetInstanceExtensions(window_, &extensionCount, extensions.data());

  return extensions;
}

void GraphicsDevice::SetAppInfo(vk::ApplicationInfo& appInfo) const {
  appInfo.setPNext(nullptr)
      .setPApplicationName("Vulkan Engine")
      .setApplicationVersion(0)
      .setApiVersion(VK_API_VERSION_1_1)
      .setPEngineName("None")
      .setEngineVersion(0);
}

bool GraphicsDevice::IsPhysicalDeviceSuitable(
    const vk::PhysicalDevice& curGpu) const {
  auto properties = curGpu.getProperties();
  auto features = curGpu.getFeatures();
  return properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
}

std::vector<const char*> GraphicsDevice::GetDeviceExtensions() const {
  return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
}

std::vector<QueueReference> GraphicsDevice::GetDeviceQueueInfos() {
  std::vector<QueueReference> infos;
  infos.emplace_back();
  infos.back().queue = &present_queue_;
  infos.back().queue_priority = UINT32_MAX;
  for (uint32_t i = 0; i < queue_family_properties_.size(); i++) {
    if (physical_device_.getSurfaceSupportKHR(i, surface_))
      infos.back().queue_family_index = i;
  }

  infos.emplace_back();
  infos.back().queue = &present_queue_;
  infos.back().queue_priority = UINT32_MAX;
  for (uint32_t i = 0; i < queue_family_properties_.size(); i++) {
    if (queue_family_properties_[i].queueFlags & vk::QueueFlagBits::eGraphics)
      infos.back().queue_family_index = i;
  }

  return infos;
}

bool GraphicsDevice::InitSurface() {
  VkSurfaceKHR cSurf;
  auto result = SDL_Vulkan_CreateSurface(window_, instance_, &cSurf);
  if (result == SDL_TRUE) surface_ = cSurf;

  return result == SDL_TRUE;
}

} // namespace VPP
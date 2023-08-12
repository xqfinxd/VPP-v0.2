#pragma once

#include "GraphicsDevice.h"

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>
#include <map>

namespace VPP {

GraphicsDevice::GraphicsDevice(SDL_Window* window) {
  assert(window);
  assert(SDL_GetWindowFlags(window) & SDL_WINDOW_VULKAN);
  m_Window = window;

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
  if (m_Instance && m_Surface) m_Instance.destroy(m_Surface);
}

std::vector<const char*> GraphicsDevice::GetInstanceExtensions() const {
  std::vector<const char*> extensions;
  uint32_t extensionCount = 0;
  SDL_Vulkan_GetInstanceExtensions(m_Window, &extensionCount, nullptr);
  assert(extensionCount);
  extensions.resize(extensionCount);
  SDL_Vulkan_GetInstanceExtensions(m_Window, &extensionCount, extensions.data());

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
  infos.back().m_QueueObject = &m_PresentQueue;
  infos.back().m_QueuePriority = UINT32_MAX;
  for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++) {
    if (m_PhysicalDevice.getSurfaceSupportKHR(i, m_Surface))
      infos.back().m_QueueFamilyIndex = i;
  }

  infos.emplace_back();
  infos.back().m_QueueObject = &m_PresentQueue;
  infos.back().m_QueuePriority = UINT32_MAX;
  for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++) {
    if (m_QueueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)
      infos.back().m_QueueFamilyIndex = i;
  }

  return infos;
}

bool GraphicsDevice::InitSurface() {
  VkSurfaceKHR cSurf;
  auto result = SDL_Vulkan_CreateSurface(m_Window, m_Instance, &cSurf);
  if (result == SDL_TRUE) m_Surface = cSurf;

  return result == SDL_TRUE;
}

} // namespace VPP
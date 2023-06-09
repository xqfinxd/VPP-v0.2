#include "Renderer.h"

#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <set>

#include "Variables.h"

namespace VPP {
namespace impl {
static VkBool32 DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      level,
    VkDebugUtilsMessageTypeFlagsEXT             type,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                       pUserData) {
  if (level & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    return VK_TRUE;
  }
  std::cerr << "[vulkan] ";
  switch (level) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      std::cerr << "Warn: ";
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      std::cerr << "Error: ";
      break;
    default:
      break;
  }
  std::cerr << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

static std::vector<const char*> GetWindowExtensions(SDL_Window* window) {
  std::vector<const char*> extensions{};

  uint32_t extensionCount = 0;
  if (SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr) ==
      SDL_TRUE) {
    extensions.resize(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount,
                                     extensions.data());
  }

  return extensions;
}

Renderer::Renderer(Window& window) {
  CreateInstance(window.window_);
  CreateSurface(window.window_);
  SetGpuAndIndices();
  CreateDevice();
  GetQueues();
}

Renderer::~Renderer() {
  if (device) {
    device.destroy();
  }
  if (instance) {
    if (surface) {
      instance.destroy(surface);
    }
    instance.destroy();
  }
}

bool Renderer::FindMemoryType(uint32_t memType, vk::MemoryPropertyFlags mask,
                              uint32_t& typeIndex) const {
  if (!gpu) {
    return false;
  }
  auto props = gpu.getMemoryProperties();
  for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
    if ((memType & 1) == 1) {
      if ((props.memoryTypes[i].propertyFlags & mask) == mask) {
        typeIndex = i;
        return true;
      }
    }
    memType >>= 1;
  }

  return false;
}

void Renderer::CreateInstance(SDL_Window* window) {
  vk::Result result = vk::Result::eSuccess;

  auto appCI = vk::ApplicationInfo()
                   .setPNext(nullptr)
                   .setPApplicationName("Vulkan Engine")
                   .setApplicationVersion(0)
                   .setApiVersion(VK_API_VERSION_1_1)
                   .setPEngineName("None")
                   .setEngineVersion(0);

  auto  extensions = GetWindowExtensions(window);
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  std::vector<const char*> enabledLayers{};
  for (auto& layer : g_Vars.layers) {
    enabledLayers.push_back(layer.c_str());
  }

  using MsgSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
  using MsgType = vk::DebugUtilsMessageTypeFlagBitsEXT;
  auto debugCI =
      vk::DebugUtilsMessengerCreateInfoEXT()
          .setPNext(nullptr)
          .setMessageSeverity(MsgSeverity::eInfo | MsgSeverity::eWarning |
                              MsgSeverity::eError)
          .setMessageType(MsgType::eGeneral | MsgType::eValidation |
                          MsgType::ePerformance)
          .setPfnUserCallback(DebugCallback);

  auto instCI = vk::InstanceCreateInfo()
                    .setPEnabledLayerNames(enabledLayers)
                    .setPEnabledExtensionNames(extensions)
                    .setPApplicationInfo(&appCI)
                    .setPNext(&debugCI);

  result = vk::createInstance(&instCI, nullptr, &instance);
  assert(result == vk::Result::eSuccess);
}

void Renderer::CreateSurface(SDL_Window* window) {
  VkSurfaceKHR cSurf;
  SDL_Vulkan_CreateSurface(window, instance, &cSurf);
  surface = cSurf;
  assert(surface);
}

void Renderer::SetGpuAndIndices() {
  auto availableGPUs = instance.enumeratePhysicalDevices();
  bool found = false;

  for (const auto& curGpu : availableGPUs) {
    auto properties = curGpu.getProperties();
    auto features = curGpu.getFeatures();
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      gpu = curGpu;
      found = true;
      break;
    }
  }
  assert(found);

  auto     queueProperties = gpu.getQueueFamilyProperties();
  uint32_t indexCount = (uint32_t)queueProperties.size();

  std::vector<vk::Bool32> supportsPresent(indexCount);
  for (uint32_t i = 0; i < indexCount; i++) {
    gpu.getSurfaceSupportKHR(i, surface, &supportsPresent[i]);
  }

  for (uint32_t i = 0; i < indexCount; ++i) {
    if (queueProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
      indices.graphics = i;
    }

    if (supportsPresent[i] == VK_TRUE) {
      indices.present = i;
    }

    if (indices.HasValue()) {
      break;
    }
  }
  assert(indices.HasValue());
}

void Renderer::CreateDevice() {
  vk::Result result = vk::Result::eSuccess;

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
  std::set<uint32_t> queueFamilies = {indices.graphics, indices.present};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : queueFamilies) {
    auto queueCreateInfo = vk::DeviceQueueCreateInfo()
                               .setQueueFamilyIndex(queueFamily)
                               .setQueueCount(1)
                               .setPQueuePriorities(&queuePriority);
    queueCreateInfos.push_back(queueCreateInfo);
  }

  std::vector<const char*> enabledLayers{};
  for (auto& layer : g_Vars.layers) {
    enabledLayers.push_back(layer.c_str());
  }

  std::vector<const char*> enabledExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  vk::DeviceCreateInfo deviceCI =
      vk::DeviceCreateInfo()
          .setQueueCreateInfoCount(1)
          .setQueueCreateInfos(queueCreateInfos)
          .setPEnabledExtensionNames(enabledExtensions)
          .setPEnabledLayerNames(enabledLayers)
          .setPEnabledFeatures(nullptr);

  result = gpu.createDevice(&deviceCI, nullptr, &device);
  assert(result == vk::Result::eSuccess);
}

void Renderer::GetQueues() {
  queues.graphics = device.getQueue(indices.graphics, 0);
  queues.present = device.getQueue(indices.present, 0);
}
}  // namespace impl
}  // namespace VPP
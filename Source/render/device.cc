#include "render/device.h"
#include "render/device_d.h"

#include <iostream>

#include <SDL2/SDL_vulkan.h>

#include "public/console.h"
#include "window/window_d.h"

static VkBool32 DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT level,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
  switch (level) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      std::cerr << Console::log << "Log:";
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      std::cerr << Console::warn << "Warn:";
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      std::cerr << Console::error << "Error:";
      break;
    default:
      std::cerr << Console::clear;
      break;
  }

  std::cerr << Console::log << pCallbackData->pMessage;
  std::cerr << Console::clear << std::endl;

  return VK_FALSE;
}

void Renderer_D::initContext() {}
void Renderer_D::initDevice() {}
void Renderer_D::initSwapchain() {}
void Renderer_D::initRenderPass() {}

Renderer::Renderer() {
    initImpl();
}

Renderer::~Renderer() {

}

void Renderer::bindWindow(Window& window) {
    getImpl()->window = window.shareImpl();
    getImpl()->initContext();
    getImpl()->initDevice();
}

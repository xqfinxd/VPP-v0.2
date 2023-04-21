#include "window.h"

#include <cassert>
#include <GLFW/glfw3.h>

#include "config.h"
#include "device.h"

vk::Extent2D MainWindow::getSurfExtent() const {
  int w = 0, h = 0;
  glfwGetFramebufferSize(window, &w, &h);
  return VkExtent2D{(uint32_t)w, (uint32_t)h};
}

vk::SurfaceKHR MainWindow::getSurface(VkInstance instance) const {
  VkSurfaceKHR surface;
  auto res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
  assert(res == VK_SUCCESS);
  return vk::SurfaceKHR(surface);
}

std::vector<ExtensionType> MainWindow::getExtensions() const {
  std::vector<const char*> extensions{};
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  extensions.assign(glfwExtensions, glfwExtensions + glfwExtensionCount);

  return extensions;
}

void MainWindow::run() {
  if (!window) return;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

MainWindow::MainWindow() {
  int res = GLFW_TRUE;
  res = glfwInit();
  assert(GLFW_TRUE == res);
  res = glfwVulkanSupported();
  assert(GLFW_TRUE == res);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  auto* pCfg = Config::Get();
  window = glfwCreateWindow((int)pCfg->toInteger("window.width"),
                             (int)pCfg->toInteger("window.height"),
                             pCfg->toString("window.title"), NULL, NULL);
  assert(window);
}

MainWindow::~MainWindow() { glfwDestroyWindow(window); }
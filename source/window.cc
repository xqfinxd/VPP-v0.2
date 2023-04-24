#include "window.h"

#include <GLFW/glfw3.h>

#include "config.h"
#include "device.h"

vk::Extent2D MainWindow::getSurfExtent() const {
  int w = 0, h = 0;
  glfwGetFramebufferSize(window, &w, &h);
  return VkExtent2D{(uint32_t)w, (uint32_t)h};
}

vk::SurfaceKHR MainWindow::getSurface(vk::Instance instance) const {
  VkSurfaceKHR surface;
  auto res =
      glfwCreateWindowSurface((VkInstance)instance, window, nullptr, &surface);
  IFNO_THROW(res == VK_SUCCESS, "fail to create surface");
  return vk::SurfaceKHR(surface);
}

std::vector<const char*> MainWindow::getExtensions() const {
  std::vector<const char*> extensions{};
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  extensions.assign(glfwExtensions, glfwExtensions + glfwExtensionCount);

  return extensions;
}

void MainWindow::run() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

MainWindow::MainWindow() {
  int res = GLFW_TRUE;
  res = glfwInit();
  IFNO_THROW(GLFW_TRUE == res, "fail to init glfw");
  res = glfwVulkanSupported();
  IFNO_THROW(GLFW_TRUE == res, "vulkan is not supported");
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  if (auto section = cfg::Find("window")) {
    window = glfwCreateWindow(
        (int)section->getInteger("width"), (int)section->getInteger("height"),
        section->getString("title").c_str(), nullptr, nullptr);
  }

  IFNO_THROW(window, "fail to create window");
}

MainWindow::~MainWindow() {
  if (window) {
    glfwDestroyWindow(window);
  }
}
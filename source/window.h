#pragma once

#include <mutex>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "device.h"
#include "utility.h"

struct GLFWwindow;

class MainWindow : public Singleton<MainWindow> {
 public:
  MainWindow();
  ~MainWindow();

  vk::Extent2D getSurfExtent() const;
  vk::SurfaceKHR getSurface(VkInstance instance) const;
  std::vector<ExtensionType> getExtensions() const;
  void run();

 protected:
  GLFWwindow* window;
};

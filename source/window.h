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

  vk::Extent2D GetSurfaceExtent() const;
  vk::SurfaceKHR GetSurface(VkInstance instance) const;
  std::vector<ExtensionType> GetExtensions() const;
  void Run();

 protected:
  GLFWwindow* window_;
};

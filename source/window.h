#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "utility.h"

struct GLFWwindow;

class MainWindow : public Singleton<MainWindow> {
 public:
  MainWindow();
  ~MainWindow();

  vk::Extent2D getSurfExtent() const;
  vk::SurfaceKHR getSurface(vk::Instance instance) const;
  std::vector<const char*> getExtensions() const;
  void run();

 protected:
  GLFWwindow* window;
};

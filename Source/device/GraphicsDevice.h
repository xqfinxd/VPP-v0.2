#pragma once

#include "DeviceBase.h"

// clang-format off

struct SDL_Window;

namespace VPP {

class GraphicsDevice : public DeviceBase {
public:
  GraphicsDevice(SDL_Window* window);
  virtual ~GraphicsDevice();
  
protected:
  std::vector<const char*> GetInstanceExtensions() const;
  void SetAppInfo(vk::ApplicationInfo& appInfo) const;
  bool IsPhysicalDeviceSuitable(const vk::PhysicalDevice&) const;

  std::vector<const char*> GetDeviceExtensions() const;
  std::vector<QueueReference> GetDeviceQueueInfos();
  bool InitSurface();

protected:
  SDL_Window*       window_;
  vk::SurfaceKHR    surface_;
  QueueObject       graphics_queue_;
  QueueObject       present_queue_;
};

}

// clang-format on
#pragma once

#include "Renderer.h"
#include "Singleton.hpp"
#include "Swapchain.h"

namespace VPP {
namespace impl {

class Device : protected std::enable_shared_from_this<Device>,
               protected Singleton<Device> {
  friend class DeviceResource;
  friend Device* GetDevice();

 public:
  Device(std::shared_ptr<Window> window);
  ~Device();

 private:
  std::shared_ptr<Window> window_{};
  Renderer*               renderer_ = nullptr;
  Swapchain*              swapchain_ = nullptr;
};

Device* GetDevice();

class DeviceResource {
 protected:
  DeviceResource();
  inline Renderer*  renderer();

 private:
  std::shared_ptr<Device> device_;
};

}  // namespace impl
}  // namespace VPP
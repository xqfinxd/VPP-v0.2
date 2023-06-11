#include "Device.h"

namespace VPP {
namespace impl {

Device::Device(std::shared_ptr<Window> window) : window_(window) {
  assert(window_);
  renderer_ = new Renderer(*window_.get());
  swapchain_ = new Swapchain(renderer_);
}

Device::~Device() {
  delete swapchain_;
  delete renderer_;
}

extern Device* GetDevice() {
  return Device::GetSingleton();
}

DeviceResource::DeviceResource() {
  device_ = GetDevice()->shared_from_this();
}

VPP::impl::Renderer* DeviceResource::renderer() {
  return device_->renderer_;
}

inline Swapchain* DeviceResource::swapchain() {
  return device_->swapchain_;
}

}  // namespace impl
}  // namespace VPP
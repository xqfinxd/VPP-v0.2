#pragma once

#include <vulkan/vulkan.hpp>

class VPPDeviceBase {
public:
  VPPDeviceBase();
  virtual ~VPPDeviceBase();

protected:


protected:
  vk::Instance instance_;
  vk::PhysicalDevice physical_device_;
  vk::Device device_;
};

#pragma once

#include <vulkan/vulkan.hpp>

// clang-format off

namespace vpp {

struct DeviceQueueBase {
  uint32_t  queue_family_index = UINT32_MAX;
  uint32_t  queue_priority = 0;
  vk::Queue queue = nullptr;
};

struct DeviceQueueInfo {
  uint32_t          queue_family_index = UINT32_MAX;
  uint32_t          queue_priority = 0;
  DeviceQueueBase*  queue = nullptr;
};

class DeviceBase {
public:
  virtual ~DeviceBase();

protected:
  DeviceBase() {}

  vk::Result InitInstance();
  bool InitPhysicalDevice();
  vk::Result InitDevice();

  virtual std::vector<const char*> GetInstanceExtensions() const;
  virtual std::vector<const char*> GetInstanceLayers() const;
  virtual void SetAppInfo(vk::ApplicationInfo& appInfo) const = 0;
  virtual bool IsPhysicalDeviceSuitable(const vk::PhysicalDevice&) const = 0;

  virtual std::vector<const char*> GetDeviceExtensions() const;
  virtual std::vector<const char*> GetDeviceLayers() const;
  virtual std::vector<DeviceQueueInfo> GetDeviceQueueInfos();

protected:
  vk::Instance                           instance_;
  vk::PhysicalDevice                     physical_device_;
  std::vector<vk::QueueFamilyProperties> queue_family_properties_;
  vk::Device                             device_;
  DeviceQueueBase                        base_queue_;
};

}

// clang-format on
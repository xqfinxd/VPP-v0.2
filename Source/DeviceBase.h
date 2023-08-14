#pragma once

#include <vulkan/vulkan.hpp>

// clang-format off

namespace VPP {

struct QueueObject {
  uint32_t  m_QueueFamilyIndex = UINT32_MAX;
  uint32_t  m_QueuePriority = 0;
  vk::Queue m_Queue = nullptr;
};

struct QueueReference {
  uint32_t          m_QueueFamilyIndex = UINT32_MAX;
  uint32_t          m_QueuePriority = 0;
  QueueObject*      m_QueueObject = nullptr;
};

class DeviceBase {
public:
  virtual ~DeviceBase();
  const vk::PhysicalDevice& gpu() const {return m_PhysicalDevice;}
  const vk::Device& device() const {return m_Device;}

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
  virtual std::vector<QueueReference> GetDeviceQueueInfos();

protected:
  vk::Instance                            m_Instance;
  vk::PhysicalDevice                      m_PhysicalDevice;
  std::vector<vk::QueueFamilyProperties>  m_QueueFamilyProperties;
  vk::Device                              m_Device;
  QueueObject                             m_BasicQueue;
};

}

// clang-format on
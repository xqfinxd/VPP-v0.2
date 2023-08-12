#pragma once

#include "DeviceBase.h"

#include <map>

namespace VPP {

DeviceBase::~DeviceBase() {
  if (m_Device) m_Device.destroy();
  if (m_Instance) m_Instance.destroy();
}

vk::Result DeviceBase::InitInstance() {
  auto appCI = vk::ApplicationInfo();
  SetAppInfo(appCI);

  auto extensions = GetInstanceExtensions();
  auto layers = GetInstanceLayers();

  auto instCI = vk::InstanceCreateInfo()
                    .setPEnabledLayerNames(layers)
                    .setPEnabledExtensionNames(extensions)
                    .setPApplicationInfo(&appCI);

  return vk::createInstance(&instCI, nullptr, &m_Instance);
}

bool DeviceBase::InitPhysicalDevice() {
  auto allGpus = m_Instance.enumeratePhysicalDevices();

  for (const auto& curGpu : allGpus) {
    if (!IsPhysicalDeviceSuitable(curGpu)) continue;

    uint32_t queueFamilyCount = 0;
    curGpu.getQueueFamilyProperties(&queueFamilyCount, nullptr);
    if (!queueFamilyCount) continue;

    m_QueueFamilyProperties.reserve(queueFamilyCount);
    m_QueueFamilyProperties = curGpu.getQueueFamilyProperties();

    m_PhysicalDevice = curGpu;

    return true;
  }

  return false;
}

vk::Result DeviceBase::InitDevice() {
  struct _TempQueueInfo {
    QueueObject* base;
    uint32_t index;
  };
  std::vector<_TempQueueInfo> queueInfoCaches;
  std::map<uint32_t, std::vector<float>> queuePriorities;

  {
    auto queueInfos = GetDeviceQueueInfos();
    uint32_t transfer_queue_family_index = 0;
    for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); ++i) {
      if (m_QueueFamilyProperties[i].queueFlags &
          vk::QueueFlagBits::eTransfer) {
        transfer_queue_family_index = i;
        break;
      }
    }
    queueInfos.push_back(
        QueueReference{transfer_queue_family_index, UINT32_MAX, &m_BasicQueue});

    uint32_t _base = 0, _range = UINT32_MAX;
    {
      uint32_t maxPriority = 0, minPriority = UINT32_MAX;
      std::for_each(queueInfos.begin(), queueInfos.end(),
                    [&maxPriority, &minPriority](const QueueReference& info) {
                      if (info.m_QueuePriority > maxPriority)
                        maxPriority = info.m_QueuePriority;
                      if (info.m_QueuePriority < minPriority)
                        minPriority = info.m_QueuePriority;
                    });
      if (maxPriority - minPriority != 0) {
        _base = minPriority;
        _range = maxPriority - minPriority;
      }
    }
    auto Norm = [&_base, &_range](uint32_t priorpity) {
      return 1.f * (priorpity - _base) / _range;
    };

    for (size_t i = 0; i < queueInfos.size(); i++) {
      auto& info = queueInfos[i];
      auto& qp = queuePriorities[info.m_QueueFamilyIndex];

      info.m_QueueObject->m_QueueFamilyIndex = info.m_QueueFamilyIndex;
      info.m_QueueObject->m_QueuePriority = info.m_QueuePriority;

      auto normalizedPriority = Norm(info.m_QueuePriority);
      qp.push_back(normalizedPriority);

      queueInfoCaches.emplace_back();
      queueInfoCaches.back().base = info.m_QueueObject;
      queueInfoCaches.back().index = (uint32_t)qp.size() - 1;
    }
  }

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
  for (const auto& qp : queuePriorities) {
    assert(qp.first < m_QueueFamilyProperties.size());
    assert(qp.second.size() < m_QueueFamilyProperties[qp.first].queueCount);

    auto queueCreateInfo = vk::DeviceQueueCreateInfo()
                               .setQueueFamilyIndex(qp.first)
                               .setQueueCount((uint32_t)qp.second.size())
                               .setPQueuePriorities(qp.second.data());
    queueCreateInfos.push_back(queueCreateInfo);
  }

  auto extensions = GetDeviceExtensions();
  auto layers = GetDeviceLayers();

  vk::DeviceCreateInfo deviceCI = vk::DeviceCreateInfo()
                                      .setQueueCreateInfoCount(1)
                                      .setQueueCreateInfos(queueCreateInfos)
                                      .setPEnabledExtensionNames(extensions)
                                      .setPEnabledLayerNames(layers)
                                      .setPEnabledFeatures(nullptr);

  auto result = m_PhysicalDevice.createDevice(&deviceCI, nullptr, &m_Device);
  if (result == vk::Result::eSuccess) {
    for (auto& cache : queueInfoCaches) {
      m_Device.getQueue(cache.base->m_QueueFamilyIndex, cache.index,
                       &cache.base->m_Queue);
    }
  }

  return result;
}

std::vector<const char*> DeviceBase::GetInstanceExtensions() const {
  return {};
}

std::vector<const char*> DeviceBase::GetInstanceLayers() const {
  return {
      "VK_LAYER_KHRONOS_validation",
  };
}

std::vector<const char*> DeviceBase::GetDeviceExtensions() const {
  return {};
}

std::vector<const char*> DeviceBase::GetDeviceLayers() const {
  return {
      "VK_LAYER_KHRONOS_validation",
  };
}

std::vector<QueueReference> DeviceBase::GetDeviceQueueInfos() {
  return {};
}

} // namespace VPP
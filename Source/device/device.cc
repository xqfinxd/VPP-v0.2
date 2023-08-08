#pragma once

#include "device.h"

#include <map>

namespace vpp {

DeviceBase::~DeviceBase() {
  if (device_) device_.destroy();
  if (instance_) instance_.destroy();
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

  return vk::createInstance(&instCI, nullptr, &instance_);
}

bool DeviceBase::InitPhysicalDevice() {
  auto allGpus = instance_.enumeratePhysicalDevices();

  for (const auto& curGpu : allGpus) {
    if (!IsPhysicalDeviceSuitable(curGpu)) continue;

    uint32_t queueFamilyCount = 0;
    curGpu.getQueueFamilyProperties(&queueFamilyCount, nullptr);
    if (!queueFamilyCount) continue;

    queue_family_properties_.reserve(queueFamilyCount);
    queue_family_properties_ = curGpu.getQueueFamilyProperties();

    physical_device_ = curGpu;

    return true;
  }

  return false;
}

vk::Result DeviceBase::InitDevice() {
  struct QueueInfoCache {
    DeviceQueueBase* base;
    uint32_t index;
  };
  std::vector<QueueInfoCache> queueInfoCaches;
  std::map<uint32_t, std::vector<float>> queuePriorities;

  {
    auto queueInfos = GetDeviceQueueInfos();
    uint32_t transfer_queue_family_index = 0;
    for (uint32_t i = 0; i < queue_family_properties_.size(); ++i) {
      if (queue_family_properties_[i].queueFlags &
          vk::QueueFlagBits::eTransfer) {
        transfer_queue_family_index = i;
        break;
      }
    }
    queueInfos.push_back(
        DeviceQueueInfo{transfer_queue_family_index, UINT32_MAX, &base_queue_});

    uint32_t _base = 0, _range = UINT32_MAX;
    {
      uint32_t maxPriority = 0, minPriority = UINT32_MAX;
      std::for_each(queueInfos.begin(), queueInfos.end(),
                    [&maxPriority, &minPriority](const DeviceQueueInfo& info) {
                      if (info.queue_priority > maxPriority)
                        maxPriority = info.queue_priority;
                      if (info.queue_priority < minPriority)
                        minPriority = info.queue_priority;
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
      auto& qp = queuePriorities[info.queue_family_index];

      info.queue->queue_family_index = info.queue_family_index;
      info.queue->queue_priority = info.queue_priority;

      auto normalizedPriority = Norm(info.queue_priority);
      qp.push_back(normalizedPriority);

      queueInfoCaches.emplace_back();
      queueInfoCaches.back().base = info.queue;
      queueInfoCaches.back().index = (uint32_t)qp.size() - 1;
    }
  }

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
  for (const auto& qp : queuePriorities) {
    assert(qp.first < queue_family_properties_.size());
    assert(qp.second.size() < queue_family_properties_[qp.first].queueCount);

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

  auto result = physical_device_.createDevice(&deviceCI, nullptr, &device_);
  if (result == vk::Result::eSuccess) {
    for (auto& cache : queueInfoCaches) {
      device_.getQueue(cache.base->queue_family_index, cache.index,
                       &cache.base->queue);
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

std::vector<DeviceQueueInfo> DeviceBase::GetDeviceQueueInfos() {
  return {};
}

} // namespace vpp
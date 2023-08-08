#include <iostream>

#include "VPP/Application.h"
#include "device/device.h"

class Device : public vpp::DeviceBase {
public:
  Device() {
    InitInstance();
    InitPhysicalDevice();
    InitDevice();
  }
  ~Device() {
  
  }

private:
  virtual void SetAppInfo(vk::ApplicationInfo& appInfo) const {
    appInfo.setPApplicationName("Vulkan Engine")
        .setApplicationVersion(0)
        .setApiVersion(VK_API_VERSION_1_1)
        .setPEngineName("None")
        .setEngineVersion(0);
  }

  virtual bool IsPhysicalDeviceSuitable(const vk::PhysicalDevice& gpu) const {
    auto properties = gpu.getProperties();
    auto features = gpu.getFeatures();
    return properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
  }

};


class MyApp : public VPP::Application {
public:
  MyApp() {}
  ~MyApp() {}

  void OnLoop() override { __super::OnLoop(); }

private:
};

int main(int argc, char** argv) {
  Device deivce;
  MyApp app{};
  app.Run();
}
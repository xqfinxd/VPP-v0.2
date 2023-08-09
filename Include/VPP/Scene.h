#pragma once

#include "camera.h"

#include <map>
#include <string>

namespace VPP {

class Scene {
public:
  Scene() {}
  ~Scene() {
    for (auto& camera : cameras_) {
      delete camera.second;
    }
    cameras_.clear();
  }

  void Run() {}
  void Draw() {}

  Camera* AddCamera(const char* name) {
    auto iter = cameras_.find(name);
    if (iter == cameras_.end()) {
      auto camera = new Camera;
      cameras_[name] = camera;
      return camera;
    }
    return nullptr;
  }

  void RemoveCamera(const char* name) {
    auto iter = cameras_.find(name);
    if (iter != cameras_.end()) {
      delete iter->second;
      cameras_.erase(iter);
    }
  }

  Camera* FindCamera(const char* name) {
    auto iter = cameras_.find(name);
    if (iter != cameras_.end()) {
      return iter->second;
    }
    return nullptr;
  }

private:
  glm::vec3 world_up{0.f, 1.f, 0.f};
  std::map<std::string, Camera*> cameras_;
};

} // namespace VPP
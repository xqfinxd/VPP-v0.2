#pragma once

#include "GameObject.h"

#include <map>
#include <string>

namespace VPP {

class Scene : public GameObjectManager {
public:
  Scene() {}
  ~Scene() {
    
  }

  void Run() {}
  void Draw() {}

private:
  glm::vec3 world_up{0.f, 1.f, 0.f};
};

} // namespace VPP
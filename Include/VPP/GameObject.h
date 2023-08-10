#pragma once

#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Component.h"

namespace VPP {

class Scene;

struct Transform {
  glm::vec3 postion{0.f};
  glm::vec3 rotation{0.f};
  glm::vec3 scale{1.f};
};

class GameObject : public ComponentManager {
  friend class GameObjectManager;

public:
  GameObject() : ComponentManager(this) {}
  ~GameObject() {}

  Transform& transform() {
    return transform_;
  }

  const char* name() const {
    return name_.c_str();
  }
  void SetName(const char* newName) {
    name_.assign(newName);
  }

private:
  bool enable_ = true;
  std::string name_;
  Transform transform_;
};

class GameObjectManager {
public:
  GameObjectManager() {}
  ~GameObjectManager() {}

  GameObject* AddGameObject() {
    game_objects_.emplace_back(new GameObject);
    return game_objects_.back().get();
  }

  void RemoveGameObject(GameObject* gameObject) {
    for (auto iter = game_objects_.begin(); iter != game_objects_.end();
         ++iter) {
      if (iter->get() == gameObject) {
        game_objects_.erase(iter);
        return;
      }
    }
  }

private:
  using _GameObject = std::unique_ptr<GameObject>;
  std::vector<_GameObject> game_objects_;
};

} // namespace VPP
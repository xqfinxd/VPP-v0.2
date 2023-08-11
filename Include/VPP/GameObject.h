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

  Scene* GetScene() {
    return scene_;
  }

  const Scene* GetScene() const {
    return scene_;
  }

  const char* name() const {
    return name_.c_str();
  }
  void SetName(const char* newName) {
    name_.assign(newName);
  }

protected:
  bool enable_ = true;
  std::string name_;
  Transform transform_;

private:
  Scene* scene_ = nullptr;
};

class GameObjectManager {
public:
  GameObjectManager(Scene* scene) : scene_(scene) {}
  ~GameObjectManager() {}

  GameObject* AddGameObject() {
    auto newGO = new GameObject;
    auto pointer = reinterpret_cast<intptr_t>(newGO);
    game_objects_[pointer].reset(newGO);
    sort_ids_.push_back(pointer);
    newGO->scene_ = scene_;
    return newGO;
  }

  void RemoveGameObject(intptr_t pointer) {
    game_objects_.erase(pointer);
    
    auto id_iter = std::find(sort_ids_.begin(), sort_ids_.end(), pointer);
    if (id_iter != sort_ids_.end()) sort_ids_.erase(id_iter);
  }

  GameObject* FindGameObject(intptr_t pointer) {
    auto iter = game_objects_.find(pointer);
    if (iter != game_objects_.end()) {
      return iter->second.get();
    }

    return nullptr;
  }

private:
  using _GameObject = std::unique_ptr<GameObject>;
  std::map<intptr_t, _GameObject> game_objects_;
  std::vector<intptr_t> sort_ids_;
  Scene* scene_ = nullptr;
};

} // namespace VPP
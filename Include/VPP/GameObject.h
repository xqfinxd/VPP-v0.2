#pragma once

#include <map>
#include <glm/glm.hpp>

#include "Component.h"

namespace VPP {

class Component;

struct Transform {
  glm::vec3 postion{0.f};
  glm::vec3 rotation{0.f};
  glm::vec3 scale{1.f};
};

class GameObject {
public:
  const Transform& transform() const {
    return transform_;
  }
  Transform& transform() {
    return transform_;
  }

  template <class Comp> Comp* AddComponent() {
    auto id = GetComponentID<Comp>();
    auto iter = components_.find(id);
    if (iter == components_.end()) {
      auto comp = new Comp;
      comp->game_object_ = this;
      components_[id] = comp;
      return comp;
    }

    return nullptr;
  }

  template <class Comp> void RemoveComponent() {
    auto id = GetComponentID<Comp>();
    auto iter = components_.find(id);
    if (iter != components_.end()) {
      delete iter->second;
      components_.erase(iter);
    }
  }

  template <class Comp> Comp* GetComponent() {
    auto id = GetComponentID<Comp>();
    auto iter = components_.find(id);
    if (iter != components_.end()) {
      return dynamic_cast<Comp*>(iter->second);
    }

    return nullptr;
  }

private:
  bool enable_ = true;
  Transform transform_;
  std::map<size_t, Component*> components_;
};

} // namespace VPP
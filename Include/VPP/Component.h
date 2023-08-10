#pragma once

#include <type_traits>
#include <memory>
#include <vector>
#include <map>

// clang-format off

namespace VPP {

class GameObject;

class Component {
  friend class ComponentManager;

public:
  Component() {}
  virtual ~Component() {}

  GameObject* GetGameObject() {
    return game_object_;
  }

  const GameObject* GetGameObject() const {
    return game_object_;
  }

  virtual void Awake() {}
  virtual void Start() {}
  virtual void Update() {}

protected:
  bool enable_ = true;

private:
  GameObject* game_object_ = nullptr;
};

size_t AssignComponentID() {
  static size_t _IDCounter = 1;
  return _IDCounter++;
}

template <class Comp>
typename std::enable_if<std::is_base_of_v<Component, Comp>, size_t>::type
GetComponentID() {
  static const size_t _ThisID = AssignComponentID();
  return _ThisID;
}

class ComponentManager {
public:
  ~ComponentManager() {}

  template <class Comp> Comp* AddComponent() {
    auto id = GetComponentID<Comp>();
    auto iter = components_.find(id);
    if (iter == components_.end()) {
      auto newComp = new Comp;
      newComp->game_object_ = owner_;
      components_[id].reset(newComp);
      sort_ids_.push_back(id);

      newComp->Awake();
      return newComp;
    }

    return nullptr;
  }

  template <class Comp> void RemoveComponent() {
    auto id = GetComponentID<Comp>();
    auto iter = components_.find(id);
    if (iter != components_.end()) {
      components_.erase(iter);

      auto id_iter = std::find(sort_ids_.begin(), sort_ids_.end(), id);
      if (id_iter != sort_ids_.end()) sort_ids_.erase(id_iter);
    }
  }

  template <class Comp> Comp* GetComponent() {
    auto id = GetComponentID<Comp>();
    auto iter = components_.find(id);
    if (iter != components_.end()) {
      return dynamic_cast<Comp*>(iter->second.get());
    }

    return nullptr;
  }

protected:
  ComponentManager(GameObject* owner) : owner_ (owner) {}

private:
  using _Component = std::unique_ptr<Component>;
  std::map<size_t, _Component>  components_;
  std::vector<size_t>           sort_ids_;
  GameObject*                   owner_;
};

} // namespace VPP

// clang-format on
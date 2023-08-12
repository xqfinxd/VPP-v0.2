#pragma once

#include <map>
#include <memory>
#include <type_traits>
#include <vector>

// clang-format off

namespace VPP {

class GameObject;

class Component {
  friend class ComponentManager;

public:
  Component() {}
  virtual ~Component() {}

  GameObject* GetGameObject() {
    return m_GameObject;
  }

  const GameObject* GetGameObject() const {
    return m_GameObject;
  }

  virtual void Awake() {}
  virtual void Start() {}
  virtual void Update() {}

protected:
  bool m_Enable = true;

private:
  GameObject* m_GameObject = nullptr;
};

size_t AssignComponentID() {
  static size_t _MagicCounter = 1;
  return _MagicCounter++;
}

template <class TComp>
typename std::enable_if<
  std::is_base_of_v<Component, TComp>, size_t
>::type
GetComponentID() {
  static const size_t _MagicNumber = AssignComponentID();
  return _MagicNumber;
}

class ComponentManager {
public:
  ~ComponentManager() {}

  ComponentManager(const ComponentManager& other) {
    m_GameObject = other.m_GameObject;
  }

  ComponentManager(ComponentManager&& other) noexcept {
    m_GameObject = other.m_GameObject;
    std::swap(m_Components, other.m_Components);
    std::swap(m_SortIDs, other.m_SortIDs);
  }

  template <class TComp>
  TComp* AddComponent() {
    auto id = GetComponentID<TComp>();
    auto iter = m_Components.find(id);
    if (iter == m_Components.end()) {
      auto newComp = new TComp;
      newComp->m_GameObject = m_GameObject;
      m_Components[id].reset(newComp);
      m_SortIDs.push_back(id);

      newComp->Awake();
      return newComp;
    }

    return nullptr;
  }

  template <class TComp>
  void RemoveComponent() {
    auto id = GetComponentID<TComp>();
    auto iter = m_Components.find(id);
    if (iter != m_Components.end()) {
      m_Components.erase(iter);

      auto id_iter = std::find(m_SortIDs.begin(), m_SortIDs.end(), id);
      if (id_iter != m_SortIDs.end()) m_SortIDs.erase(id_iter);
    }
  }

  template <class TComp>
  TComp* GetComponent() {
    auto id = GetComponentID<TComp>();
    auto iter = m_Components.find(id);
    if (iter != m_Components.end()) {
      return dynamic_cast<TComp*>(iter->second.get());
    }

    return nullptr;
  }

protected:
  ComponentManager(GameObject* owner) : m_GameObject (owner) {}

private:
  using _Component = std::unique_ptr<Component>;
  std::map<size_t, _Component>  m_Components;
  std::vector<size_t>           m_SortIDs;
  GameObject*                   m_GameObject;
};

} // namespace VPP

// clang-format on
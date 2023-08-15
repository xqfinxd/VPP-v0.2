#pragma once

#include <map>
#include <memory>
#include <type_traits>
#include <vector>

// clang-format off

namespace VPP {

class GameObject;

class Component {
  friend class ComponentContainer;

public:
  Component() {}
  virtual ~Component() {}
  Component(const Component&) = delete;
  Component(Component&&) noexcept = default;

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

class ComponentContainer {
public:
  ComponentContainer(GameObject* owner) : m_Owner(owner){}
  ~ComponentContainer(){}

  template <class TComp>
  TComp* AddComponent() {
    auto id = GetComponentID<TComp>();
    auto iter = m_Components.find(id);
    if (iter == m_Components.end()) {
      auto newComp = new TComp;
      newComp->m_GameObject = m_Owner;
      m_Components[id].reset(newComp);
      m_SortComponents.push_back(id);

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

      auto id_iter = std::find(m_SortComponents.begin(), m_SortComponents.end(), id);
      if (id_iter != m_SortComponents.end()) m_SortComponents.erase(id_iter);
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

private:
  using UComponent = std::unique_ptr<Component>;
  std::map<size_t, UComponent>  m_Components;
  std::vector<size_t>           m_SortComponents;

private:
  GameObject* m_Owner;
};

} // namespace VPP

// clang-format on
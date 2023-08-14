#pragma once

#include <map>
#include <memory>
#include <type_traits>
#include <vector>

// clang-format off

namespace VPP {

class GameObject;

class Component {
  friend class GameObject;

public:
  Component() {}
  virtual ~Component() {}
  Component(const Component&) = delete;
  Component(Component&&) = default;

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

} // namespace VPP

// clang-format on
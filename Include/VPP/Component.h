#pragma once
#include <type_traits>

namespace VPP {

class GameObject;

class Component {
  friend class GameObject;

public:
  Component() {}
  virtual ~Component() {}
  static size_t AssignID() {
    static size_t _idCounter = 1;
    return _idCounter++;
  }

  GameObject* GetGameObject() {
    return game_object_;
  }

  const GameObject* GetGameObject() const {
    return game_object_;
  }

  virtual void OnStart() {}

protected:
  bool enable_ = true;

private:
  GameObject* game_object_ = nullptr;
};

template <class Comp>
typename std::enable_if<std::is_base_of_v<Component, Comp>, size_t>::type
GetComponentID() {
  static const size_t _thisComponentID = Component::AssignID();
  return _thisComponentID;
}

} // namespace VPP
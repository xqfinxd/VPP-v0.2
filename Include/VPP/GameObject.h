#pragma once

#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Component.h"
#include "Handle.h"

// clang-format off

namespace VPP {

class Scene;

struct Transform {
  glm::vec3 m_Postion{0.f};
  glm::vec3 m_Rotation{0.f};
  glm::vec3 m_Scale{1.f};
};

class GameObject : public ComponentContainer {
  friend class GameObjectContainer;

public:
  GameObject() : ComponentContainer(this) {}
  ~GameObject() {}

  Transform& GetTransform() {
    return m_Transform;
  }

  Scene* GetScene() {
    return m_Scene;
  }
  const Scene* GetScene() const {
    return m_Scene;
  }

  const char* GetName() const {
    return m_Name.c_str();
  }
  void SetName(const char* newName) {
    m_Name.assign(newName);
  }

private:
  bool        m_Enable = true;
  std::string m_Name;
  Transform   m_Transform;
  Scene*      m_Scene = nullptr;
};


struct tagGameObject{};
using GameObjectID = Handle<tagGameObject>;

class GameObjectContainer {
public:
  GameObjectContainer(Scene* owner) : m_Owner(owner) {}
  ~GameObjectContainer() {}

  GameObjectID AddGameObject(GameObject** gameObject = nullptr) {
    GameObjectID id;
    auto uPtr = m_GameObjects.Acquire(id);
    uPtr->reset(new GameObject);
    if(gameObject) *gameObject = uPtr->get();

    return id;
  }

  void RemoveGameObject(GameObjectID id) {
    if (auto pObject = m_GameObjects.Dereference(id))
      pObject->reset();
    m_GameObjects.Release(id);
  }

  GameObject* FindGameObject(GameObjectID id) {
    return m_GameObjects.Dereference(id)->get();
  }

  const GameObject* FindGameObject(GameObjectID id) const {
    return m_GameObjects.Dereference(id)->get();
  }

private:
  using UGameObject = std::unique_ptr<GameObject>;
  using HMgr = HandleManager<UGameObject, GameObjectID>;
  HMgr    m_GameObjects;

private:
  Scene*  m_Owner = nullptr;
};

} // namespace VPP

// clang-format on
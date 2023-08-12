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

class GameObject : public ComponentManager {
  friend class GameObjectManager;

public:
  GameObject() : ComponentManager(this) {}
  ~GameObject() {}

  Transform& transform() {
    return m_Transform;
  }

  Scene* GetScene() {
    return m_Scene;
  }

  const Scene* GetScene() const {
    return m_Scene;
  }

  const char* name() const {
    return m_Name.c_str();
  }
  void SetName(const char* newName) {
    m_Name.assign(newName);
  }

protected:
  bool        m_Enable = true;
  std::string m_Name;
  Transform   m_Transform;

private:
  Scene*      m_Scene = nullptr;
};


struct tagGameObject;

class GameObjectManager {
public:
  using GameObjectID = Handle<tagGameObject>;
  GameObjectManager(Scene* scene) : m_Scene(scene) {}
  ~GameObjectManager() {}

  GameObjectID AddGameObject(GameObject** gameObject = nullptr) {
    GameObjectID goID;
    auto pGo = m_GameObjects.Acquire(goID);
    if(gameObject) *gameObject = pGo;

    return goID;
  }

  void RemoveGameObject(GameObjectID id) {
    m_GameObjects.Release(id);
  }

  GameObject* FindGameObject(GameObjectID id) {
    return m_GameObjects.Dereference(id);
  }

  const GameObject* FindGameObject(GameObjectID id) const {
    return m_GameObjects.Dereference(id);
  }

private:
  using HMgr = HandleManager<GameObject, GameObjectID>;
  HMgr    m_GameObjects;
  Scene*  m_Scene = nullptr;
};

} // namespace VPP

// clang-format on
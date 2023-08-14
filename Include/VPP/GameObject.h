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

class GameObject {
  friend class GameObjectManager;

public:
  GameObject() {}
  ~GameObject() {}
  GameObject(const GameObject&) = delete;
  GameObject(GameObject&&) noexcept = default;

  template <class TComp>
  TComp* AddComponent() {
    auto id = GetComponentID<TComp>();
    auto iter = m_Components.find(id);
    if (iter == m_Components.end()) {
      auto newComp = new TComp;
      newComp->m_GameObject = this;
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
  using _Component = std::unique_ptr<Component>;
  bool                          m_Enable = true;
  std::string                   m_Name;
  Transform                     m_Transform;
  std::map<size_t, _Component>  m_Components;
  std::vector<size_t>           m_SortComponents;

  Scene*      m_Scene = nullptr;
};


struct tagGameObject{};

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
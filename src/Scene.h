#pragma once

#include <entt/entt.hpp>
#include "GameObject.h"
#include "UUID.h"

class b2World;

namespace VPP {

class GameObject;

class Scene {
public:
    Scene();
    ~Scene();

    GameObject CreateGameObject(const std::string &name = std::string());
    GameObject CreateGameObjectWithUUID(UUID uuid, const std::string &name = std::string());
    void DestroyGameObject(GameObject entity);

    GameObject FindGameObjectByName(const std::string &name);
    GameObject GetGameObjectByUUID(UUID uuid);

    void OnViewportResize(uint32_t width, uint32_t height);

    bool IsRunning() const {
        return m_IsRunning;
    }
    bool IsPaused() const {
        return m_IsPaused;
    }

    void SetPaused(bool paused) {
        m_IsPaused = paused;
    }

    template<typename... Components>
    auto GetAllGameObjectsWith() {
        return m_Registry.view<Components...>();
    }

private:
private:
    entt::registry m_Registry;
    uint32_t m_ViewportWidth = 0;
    uint32_t m_ViewportHeight = 0;
    bool m_IsRunning = false;
    bool m_IsPaused = false;
    int m_StepFrames = 0;

    std::unordered_map<UUID, entt::entity> m_EntityMap;

    friend class GameObject;
};

} // namespace VPP

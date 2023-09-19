#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Scene.h"
#include "UUID.h"

namespace VPP {

class Scene;

struct IDComponent {
    UUID ID;

    IDComponent() = default;
    IDComponent(const IDComponent &) = default;
};

struct TagComponent {
    std::string Tag;

    TagComponent() = default;
    TagComponent(const TagComponent &) = default;
    TagComponent(const std::string &tag)
        : Tag(tag) {}
};

struct Transform {
    glm::vec3 Translation = {0.0f, 0.0f, 0.0f};
    glm::vec3 Rotation = {0.0f, 0.0f, 0.0f};
    glm::vec3 Scale = {1.0f, 1.0f, 1.0f};

    Transform() = default;
    Transform(const Transform &) = default;
    Transform(const glm::vec3 &translation)
        : Translation(translation) {}

    glm::mat4 GetTransform() const;
};

class GameObject {
public:
    GameObject() = default;
    GameObject(entt::entity handle, Scene *scene);
    GameObject(const GameObject &other) = default;

    template<typename T, typename... Args>
    T &AddComponent(Args &&...args) {
        assert(!HasComponent<T>());
        T &component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
        return component;
    }

    template<typename T, typename... Args>
    T &AddOrReplaceComponent(Args &&...args) {
        T &component = m_Scene->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
        return component;
    }

    template<typename T>
    T &GetComponent() {
        return m_Scene->m_Registry.get<T>(m_EntityHandle);
    }

    template<typename T>
    bool HasComponent() {
        return nullptr != m_Scene->m_Registry.try_get<T>(m_EntityHandle);
    }

    template<typename T>
    void RemoveComponent() {
        m_Scene->m_Registry.remove<T>(m_EntityHandle);
    }

    operator bool() const {
        return m_EntityHandle != entt::null;
    }
    operator entt::entity() const {
        return m_EntityHandle;
    }
    operator uint32_t() const {
        return (uint32_t)m_EntityHandle;
    }

    UUID GetUUID() {
        return GetComponent<IDComponent>().ID;
    }
    const std::string &GetName() {
        return GetComponent<TagComponent>().Tag;
    }

    bool operator==(const GameObject &other) const {
        return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
    }

    bool operator!=(const GameObject &other) const {
        return !(*this == other);
    }

private:
    entt::entity m_EntityHandle{entt::null};
    Scene *m_Scene = nullptr;
};

} // namespace VPP
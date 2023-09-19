#include "Scene.h"

namespace VPP {

Scene::Scene() {
}

Scene::~Scene() {
}

GameObject Scene::CreateGameObject(const std::string &name) {
    return CreateGameObjectWithUUID(GenerateUUID(), name);
}

GameObject Scene::CreateGameObjectWithUUID(UUID uuid, const std::string &name) {
    GameObject gameObject = {m_Registry.create(), this};
    gameObject.AddComponent<IDComponent>(uuid);
    gameObject.AddComponent<Transform>();
    auto &tag = gameObject.AddComponent<TagComponent>();
    tag.Tag = name.empty() ? "Empty" : name;

    m_EntityMap[uuid] = gameObject;

    return gameObject;
}

void Scene::DestroyGameObject(GameObject entity) {
    m_EntityMap.erase(entity.GetUUID());
    m_Registry.destroy(entity);
}

GameObject Scene::FindGameObjectByName(const std::string &name) {
    auto view = m_Registry.view<TagComponent>();
    for(auto entity: view) {
        const TagComponent &tc = view.get<TagComponent>(entity);
        if(tc.Tag == name)
            return GameObject{entity, this};
    }
    return {};
}

GameObject Scene::GetGameObjectByUUID(UUID uuid) {
    if(m_EntityMap.find(uuid) != m_EntityMap.end())
        return {m_EntityMap.at(uuid), this};

    return {};
}

void Scene::OnViewportResize(uint32_t width, uint32_t height) {
    if(m_ViewportWidth == width && m_ViewportHeight == height)
        return;

    m_ViewportWidth = width;
    m_ViewportHeight = height;

    // TODO: viewport changed
}

} // namespace VPP
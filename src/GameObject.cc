#include "GameObject.h"

namespace VPP {

GameObject::GameObject(entt::entity handle, Scene *scene)
    : m_EntityHandle(handle), m_Scene(scene) {
}

glm::mat4 Transform::GetTransform() const {
    glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

    return glm::translate(glm::mat4(1.0f), Translation)
           * rotation
           * glm::scale(glm::mat4(1.0f), Scale);
}

} // namespace VPP

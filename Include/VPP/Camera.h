#pragma once

#include "Component.h"
#include <glm/glm.hpp>

namespace VPP {

enum class ProjectionType { eOrthographic, ePerspective };
enum class FovAxisType { eVertical, eHorizontal };

union ProjectionData {
  struct {
    float size;
  } orthographic;

  struct {
    float fov;
    FovAxisType fov_axis;
  } perspective;
};

struct Camera : public Component {
public:
  Camera() {
    projection_data.perspective.fov_axis = FovAxisType::eVertical;
    projection_data.perspective.fov = 60;
  }
  ~Camera() {}

private:
  glm::vec3 position{0.f, 1.f, -10.f};
  glm::vec3 rotation{0.f};
  glm::vec3 world_up{0.f, 1.f, 0.f};
  glm::vec4 clear_color{0.f};

  glm::vec2 offset{0.f};
  glm::vec2 size{1.f};

  ProjectionType projection_type = ProjectionType::ePerspective;
  ProjectionData projection_data;
  float near = 0.01f;
  float far = 1000.f;
  float depth = 0.f;
};

} // namespace VPP
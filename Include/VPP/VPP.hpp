#pragma once

#include "VPP_API.h"
#include "Scene.h"
#include "Component.h"
#include "GameObject.h"
#include "Camera.h"
#include "Mesh.h"
#include "MeshFilter.h"
#include "Shader.h"
#include "Material.h"
#include "MeshRenderer.h"

struct SDL_Window;

namespace VPP {

VPP_API void InitDevice(SDL_Window*);
VPP_API void QuitDevice();

} // namespace VPP
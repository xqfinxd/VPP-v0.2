#pragma once

#include "VPP_API.h"
#include "Camera.h"
#include "Scene.h"
#include "GameObject.h"
#include "Component.h"
#include "CubeRenderer.h"

struct SDL_Window;

namespace VPP {

VPP_API void InitDevice(SDL_Window*);
VPP_API void QuitDevice();

} // namespace VPP
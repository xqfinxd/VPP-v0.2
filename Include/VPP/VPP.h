#pragma once

#include "VPP_API.h"

struct SDL_Window;

namespace VPP {

VPP_API void InitDevice(SDL_Window*);
VPP_API void QuitDevice();

} // namespace VPP
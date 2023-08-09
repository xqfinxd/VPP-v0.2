#include "VPP.h"
#include "VPP_Core.h"

#include "device/GraphicsDevice.h"

namespace VPP {

static GraphicsDevice* g_Device = nullptr;

GraphicsDevice* GetDevice() {
  return g_Device;
}

void InitDevice(SDL_Window* window) {
  if (g_Device) return;
  g_Device = new GraphicsDevice(window);
}

void QuitDevice() {
  if (!g_Device) return;
  delete g_Device;
}

} // namespace VPP
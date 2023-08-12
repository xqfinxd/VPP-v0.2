#include "VPP.hpp"
#include "VPP_Core.h"

#include "device/GraphicsDevice.h"
#include "impl/VPPShader.h"

namespace VPP {

static GraphicsDevice* g_Device = nullptr;

GraphicsDevice* GetDevice() {
  return g_Device;
}

void InitDevice(SDL_Window* window) {
  if (g_Device) return;
  g_Device = new GraphicsDevice(window);

  auto parser = ParseGlslShaders({"basic.vert", "basic.frag"});
  parser->GetStageSpirv(vk::ShaderStageFlagBits::eFragment);
}

void QuitDevice() {
  if (!g_Device) return;
  delete g_Device;
}

} // namespace VPP
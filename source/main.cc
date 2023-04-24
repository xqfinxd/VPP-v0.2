#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <cassert>
#include <memory>

#include "config.h"
#include "device.h"
#include "shader.h"
#include "utility.h"
#include "window.h"

int main(int argc, char** argv) {
  cfg::Load("script/config.lua");

  MainWindow mainWindow{};
  Renderer renderer{};
  mainWindow.run();
  auto ptr = ShaderObject::createFromFiles(
      renderer.getDevice(),
      {
          {vk::ShaderStageFlagBits::eVertex, "texture.vert"},
          {vk::ShaderStageFlagBits::eFragment, "texture.frag"},
      });
  if (ptr) {
    ptr->destroy(renderer.getDevice());
  }

  cfg::Unload();
}
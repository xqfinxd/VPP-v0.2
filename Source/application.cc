#include "application.h"

#include <iostream>

#include "renderer_impl.h"
#include "shader_loader.h"
#include "window_impl.h"
#include "shader_impl.h"
#include "swapchain_impl.h"

namespace VPP {

Application::Application() {}

Application::~Application() {}

void Application::Run() {
  impl::Window   window{};
  impl::Renderer renderer{};
  impl::Swapchain swapchain{};

  window.set_title("VPP");
  window.set_size(1280, 720);
  window.set_fps(60);

  window.Init();
  renderer.Init();
  swapchain.Init();

  /*impl::Shader textureShader{};
  textureShader.Load({ "texture.vert", "texture.frag" });*/

  OnStart();

  impl::WindowFrame frameData{};
  while (window.running()) {
    window.StartFrame(frameData);

    OnLoop();

    window.EndFrame(frameData);
  }

  OnEnd();

  swapchain.Quit();
  renderer.Quit();
}

void Application::OnStart() {}

void Application::OnLoop() {}

void Application::OnEnd() {}

}  // namespace VPP
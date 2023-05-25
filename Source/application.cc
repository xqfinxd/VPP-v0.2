#include "application.h"

#include <iostream>

#include "renderer_impl.h"
#include "shader_loader.h"
#include "window_impl.h"

namespace VPP {

static std::unique_ptr<impl::ShaderObject> g_Shader{};

Application::Application() {}

Application::~Application() {}

void Application::Run() {
  impl::Window   window;
  impl::Renderer renderer{};
  window.set_title("VPP");
  window.set_size(1280, 720);
  window.set_fps(60);
  bool initialized = window.Init();
  assert(initialized);
  renderer.Setup();

  OnStart();

  impl::WindowFrame frameData{};
  while (window.running()) {
    window.StartFrame(frameData);

    OnLoop();

    window.EndFrame(frameData);
  }

  OnEnd();
}

void Application::OnStart() {}

void Application::OnLoop() {}

void Application::OnEnd() {}

}  // namespace VPP
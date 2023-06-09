#include "Application.h"

#include <iostream>

#include "impl/Window.h"
#include "impl/Renderer.h"

namespace VPP {

static std::shared_ptr<impl::Window> g_Window{nullptr};
impl::Renderer*                      renderer = nullptr;

Application::Application() {}

Application::~Application() {}

void Application::Run() {
  g_Window = std::make_shared<impl::Window>();
  renderer = new impl::Renderer(*g_Window.get());

  OnStart();

  impl::WindowFrame frameData{};
  while (g_Window->running()) {
    g_Window->StartFrame(frameData);

    OnLoop();

    g_Window->EndFrame(frameData);
  }

  OnEnd();

  delete renderer;
  g_Window.reset();
}

void Application::OnStart() {}

void Application::OnLoop() {}

void Application::OnEnd() {}
}  // namespace VPP
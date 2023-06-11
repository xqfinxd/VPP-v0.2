#include "Application.h"

#include <iostream>

#include "impl/Device.h"
#include "impl/Window.h"

namespace VPP {

Application::Application() {}

Application::~Application() {}

void Application::Run() {
  auto _window = std::make_shared<impl::Window>();
  auto _device = std::make_shared<impl::Device>(_window);

  OnStart();

  impl::WindowFrame frameData{};
  while (_window->running()) {
    _window->StartFrame(frameData);

    OnLoop();

    _window->EndFrame(frameData);
  }

  OnEnd();
}

void Application::OnStart() {}

void Application::OnLoop() {}

void Application::OnEnd() {}
}  // namespace VPP
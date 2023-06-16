#include "Application.h"

#include <iostream>

#include "impl/Device.h"
#include "impl/Window.h"
#include "impl/ShaderReader.h"
#include "impl/ShaderData.h"

namespace VPP {
Application::Application() {
}

Application::~Application() {
}

void Application::Run() {
  auto _window = std::make_shared<impl::Window>();
  auto _device = std::make_shared<impl::Device>(_window);

  OnStart();

  impl::WindowFrameData frameData{};
  while (_window->running()) {
    _window->StartFrame(frameData);

    OnLoop();

    _window->EndFrame(frameData);
  }

  OnEnd();
}

void Application::OnStart() {
}

void Application::OnLoop() {
}

void Application::OnEnd() {
}
}  // namespace VPP
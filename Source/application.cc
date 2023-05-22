#include "application.h"

#include <iostream>

#include "window_impl.h"
#include "renderer_impl.h"

namespace VPP {

Application::Application() {
}

Application::~Application() {
}

void Application::Run() {
    impl::Window window;
    impl::Renderer renderer{};
    window.setTitle("VPP");
    window.setSize(1280, 720);
    window.setFps(60);
    bool initialized = window.Init();
    assert(initialized);
    renderer.Setup();
    renderer.SetupContext();

    OnStart();


    impl::WindowFrame frameData{};
    while (window.Running()) {
        window.StartFrame(frameData);

        OnLoop();

        window.EndFrame(frameData);
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
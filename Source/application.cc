#include "application.h"

#include <iostream>

#include "console.h"
#include "window_impl.h"

namespace VPP {

Application::Application() {}

Application::~Application() {}

void Application::Run() {
    impl::Window window;
    window.setTitle("VPP");
    window.setSize(1280, 720);
    window.setFps(60);
    window.Init();


    window.Run();
}

}  // namespace VPP
#include "window_impl.h"
#include "console.h"

#include <iostream>

#include <SDL2/SDL_vulkan.h>

namespace VPP {

namespace impl {

Window::~Window() {
    if (window) {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
}

Window::Window() {}

bool Window::Init() {
    if (0 != SDL_Init(SDL_INIT_EVERYTHING)) {
        std::cerr << Console::error << "Error:";
        std::cerr << Console::log << SDL_GetError();
        std::cerr << Console::clear << std::endl;
        return false;
    }
    window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, size.x, size.y,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    if (!window) {
        std::cerr << Console::error << "Error:";
        std::cerr << Console::log << SDL_GetError();
        std::cerr << Console::clear << std::endl;
        return false;
    }
    return true;
}

void Window::Close() {
    runningFlag = false;
}

void Window::setSize(int width, int height) {
    size.x = width;
    size.y = height;
}

void Window::setFps(int fps) {
    if (fps) {
        frameDuration = 1000 / fps;
    }
}

void Window::setTitle(const char* title) {
    title = title;
}

void Window::Run() {
    if (!window) {
        return;
    }

    if (!frameDuration) {
        return;
    }

    runningFlag = true;
    while (runningFlag) {
        auto tickStart = SDL_GetTicks();

        SDL_Event frameEvent{};
        while (SDL_PollEvent(&frameEvent)) {
            if (SDL_QUIT == frameEvent.type) {
                runningFlag = false;
            }
        }

        auto tickEnd = SDL_GetTicks();
        auto tickDelta = tickEnd - tickStart;
        if (tickDelta < frameDuration) {
            SDL_Delay(frameDuration - tickDelta);
        }
    }
}

}  // namespace impl
}  // namespace VPP
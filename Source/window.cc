#include "window.h"
#include "console.h"
#include "window_d.h"

#include <iostream>

#include <SDL2/SDL_vulkan.h>

bool Window_D::init() {
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

Window_D::~Window_D() {
    if (window) {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
}

Window::Window() {
    initImpl();
}

Window::~Window() {}

void Window::setSize(int width, int height) {
    getImpl()->size.x = width;
    getImpl()->size.y = height;
}

void Window::setFps(int fps) {
    if (fps) {
        getImpl()->frameDuration = 1000 / fps;
    }
}

void Window::setTitle(const char* title) {
    getImpl()->title = title;
}

void Window::close() {
    getImpl()->runningFlag = false;
}

void Window::run(std::function<void(Window&)> start,
                 std::function<void(Window&)> loop,
                 std::function<void(Window&)> end) {
    if (!getImpl()->init()) {
        return;
    }

    if (start) {
        start(*this);
    }

    getImpl()->runningFlag = true;
    while (getImpl()->runningFlag) {
        auto tickStart = SDL_GetTicks();

        SDL_Event frameEvent{};
        while (SDL_PollEvent(&frameEvent)) {
            if (SDL_QUIT == frameEvent.type) {
                getImpl()->runningFlag = false;
            }
        }

        auto tickEnd = SDL_GetTicks();
        auto tickDelta = tickEnd - tickStart;
        if (tickDelta < getImpl()->frameDuration) {
            SDL_Delay(getImpl()->frameDuration - tickDelta);
        }

        if (loop) {
            loop(*this);
        }
    }

    if (end) {
        end(*this);
    }
}
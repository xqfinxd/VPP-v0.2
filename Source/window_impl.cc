#include "window_impl.h"

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

Window::Window() {
}

bool Window::Init() {
    if (0 != SDL_Init(SDL_INIT_EVERYTHING)) {
        fprintf(stderr, "[SDL2] Error: %s\n", SDL_GetError());
        return false;
    }
    window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, size.x, size.y,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    if (!window) {
        fprintf(stderr, "[SDL2] Error: %s\n", SDL_GetError());
        return false;
    }
    runningFlag = true;
    return true;
}

void Window::Close() {
    runningFlag = false;
}

void Window::StartFrame(WindowFrame& frame) {
    frame.startTicks = SDL_GetTicks();

    frame.dumpEvents.clear();
    SDL_Event frameEvent{};
    while (SDL_PollEvent(&frameEvent)) {
        if (SDL_QUIT == frameEvent.type) {
            runningFlag = false;
            break;
        }
        frame.dumpEvents.push_back(frameEvent);
    }
}

void Window::EndFrame(WindowFrame& frame) {
    auto endTicks = SDL_GetTicks();
    auto delta = endTicks - frame.startTicks;
    if (delta < frameDuration) {
        SDL_Delay(frameDuration - delta);
    }
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

}  // namespace impl

}  // namespace VPP
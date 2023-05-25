#include "window_impl.h"

#include <SDL2/SDL_vulkan.h>

#include <iostream>

namespace VPP {

namespace impl {

Window::~Window() {
  if (window_) {
    SDL_DestroyWindow(window_);
    SDL_Quit();
  }
}

Window::Window() {}

bool Window::Init() {
  if (0 != SDL_Init(SDL_INIT_EVERYTHING)) {
    fprintf(stderr, "[SDL2] Error: %s\n", SDL_GetError());
    return false;
  }
  window_ = SDL_CreateWindow(title_.c_str(), SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, size_.x, size_.y,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
  if (!window_) {
    fprintf(stderr, "[SDL2] Error: %s\n", SDL_GetError());
    return false;
  }
  running_flag_ = true;
  return true;
}

void Window::Close() {
  running_flag_ = false;
}

void Window::StartFrame(WindowFrame& frame) {
  frame.start_ticks = SDL_GetTicks();

  frame.dump_events.clear();
  SDL_Event frameEvent{};
  while (SDL_PollEvent(&frameEvent)) {
    if (SDL_QUIT == frameEvent.type) {
      running_flag_ = false;
      break;
    }
    frame.dump_events.push_back(frameEvent);
  }
}

void Window::EndFrame(WindowFrame& frame) {
  auto endTicks = SDL_GetTicks();
  auto delta = endTicks - frame.start_ticks;
  if (delta < frame_duration_) {
    SDL_Delay(frame_duration_ - delta);
  }
}

void Window::set_size(int width, int height) {
  size_.x = width;
  size_.y = height;
}

void Window::set_fps(int fps) {
  if (fps) {
    frame_duration_ = 1000 / fps;
  }
}

void Window::set_title(const char* title) {
  title = title;
}

}  // namespace impl

}  // namespace VPP
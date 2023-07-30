#include "Window.h"

#include <SDL2/SDL_vulkan.h>

#include <cassert>
#include <iostream>

namespace VPP {
namespace impl {
Window::Window(const WindowOption& option) {
  bool initialized = SDL_Init(SDL_INIT_EVERYTHING);
  assert(initialized == 0);

  window_ = SDL_CreateWindow(
      option.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      option.width, option.height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  assert(window_);
  running_flag_ = true;
  ChangeFps(option.fps);
}

Window::~Window() {
  if (window_) {
    SDL_DestroyWindow(window_);
    SDL_Quit();
  }
}

void Window::Close() { running_flag_ = false; }

void Window::StartFrame(FrameData& frame) {
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

void Window::EndFrame(FrameData& frame) {
  auto endTicks = SDL_GetTicks();
  auto delta = endTicks - frame.start_ticks;
  if (delta < frame_duration_) {
    SDL_Delay(frame_duration_ - delta);
  }
}

void Window::ChangeFps(int fps) {
  if (fps) {
    frame_duration_ = 1000 / fps;
  }
}

bool Window::IsMinimized() const {
  auto flags = SDL_GetWindowFlags(window_);
  return (flags & SDL_WINDOW_MINIMIZED) != 0;
}

} // namespace impl
} // namespace VPP
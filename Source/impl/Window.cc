#include "Window.h"

#include <SDL2/SDL_vulkan.h>
#include <cassert>
#include <iostream>

#include "Variables.h"

namespace VPP {
namespace impl {
Window::~Window() {
  if (window_) {
    SDL_DestroyWindow(window_);
    SDL_Quit();
  }
}

Window::Window() {
  bool initialized = SDL_Init(SDL_INIT_EVERYTHING);
  assert(initialized == 0);

  auto& cfg = g_Vars;
  window_ = SDL_CreateWindow(cfg.title.c_str(), SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, cfg.width, cfg.height,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
  assert(window_);
  running_flag_ = true;
  set_fps(cfg.fps);
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

void Window::set_fps(int fps) {
  if (fps) {
    frame_duration_ = 1000 / fps;
  }
}
}  // namespace impl
}  // namespace VPP
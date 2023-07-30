#pragma once

#include <SDL2/SDL.h>

#include <string>
#include <vector>

namespace VPP {
namespace impl {
struct FrameData {
  std::vector<SDL_Event> dump_events{};
  uint32_t start_ticks = 0;
  uint32_t frame_num = 0;
};

struct WindowOption {
  int width = 1280;
  int height = 900;
  int fps = 60;
  char title[64];
};

class Window {
public:
  Window(const WindowOption& option);
  ~Window();

  void Close();

  void StartFrame(FrameData& frame);
  void EndFrame(FrameData& frame);

  void ChangeFps(int fps);
  bool ShouldClose() const {
    return !running_flag_;
  }

  bool IsMinimized() const;
  SDL_Window* window() { return window_; }

  void GetSize(int& width, int& height) const {
    SDL_GetWindowSize(window_, &width, &height);
  }

private:
  bool running_flag_ = false;
  uint32_t frame_duration_ = 0;
  SDL_Window* window_ = nullptr;
};

} // namespace impl
} // namespace VPP

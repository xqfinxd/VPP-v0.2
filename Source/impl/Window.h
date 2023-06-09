#pragma once

#include <SDL2/SDL.h>

#include <string>
#include <vector>

namespace VPP {
namespace impl {
struct WindowFrame {
  std::vector<SDL_Event> dump_events{};
  uint32_t               start_ticks = 0;
  uint32_t               frame_num = 0;
};

class Window {
  friend class Renderer;

 public:
  Window();
  ~Window();

  void Close();

  void StartFrame(WindowFrame& frame);
  void EndFrame(WindowFrame& frame);

  void set_fps(int fps);
  bool running() const {
    return running_flag_;
  }

 private:
  bool        running_flag_ = false;
  uint32_t    frame_duration_ = 0;
  SDL_Window* window_ = nullptr;
};
}  // namespace impl
}  // namespace VPP

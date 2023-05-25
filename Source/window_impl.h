#pragma once

#include <SDL2/SDL.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "public/singleton.h"

namespace VPP {

namespace impl {

struct WindowFrame {
  std::vector<SDL_Event> dump_events{};
  uint32_t               start_ticks = 0;
  uint32_t               frame_num = 0;
};

class Window : public Singleton<Window> {
  friend class Renderer;

 public:
  Window();
  ~Window();

  bool Init();
  void Close();

  void StartFrame(WindowFrame& frame);
  void EndFrame(WindowFrame& frame);

  void set_size(int width, int height);
  void set_fps(int fps);
  void set_title(const char* title);
  bool running() const {
    return running_flag_;
  }

 private:
  bool        running_flag_ = false;
  uint32_t    frame_duration_ = 0;
  SDL_Window* window_ = nullptr;
  glm::ivec2  size_{0, 0};
  std::string title_{};
};

}  // namespace impl

}  // namespace VPP

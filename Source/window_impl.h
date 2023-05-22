#pragma once

#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

#include "public/singleton.h"

namespace VPP {

namespace impl {

struct WindowFrame {
    std::vector<SDL_Event> dumpEvents{};
    uint32_t               startTicks = 0;
    uint32_t               frameNum = 0;
};

class Window : public Singleton<Window> {
    friend class Renderer;

public:
    Window();
    ~Window();

    bool Init();
    void Close();
    bool Running() const {
        return runningFlag;
    }

    void StartFrame(WindowFrame& frame);
    void EndFrame(WindowFrame& frame);

    void setSize(int width, int height);
    void setFps(int fps);
    void setTitle(const char* title);

private:
    bool        runningFlag = false;
    uint32_t    frameDuration = 0;
    SDL_Window* window = nullptr;
    glm::ivec2  size{0, 0};
    std::string title{};
};

}  // namespace impl

}  // namespace VPP

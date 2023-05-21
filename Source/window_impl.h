#pragma once

#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

namespace VPP {

namespace impl {

class Window {
public:
    Window();
    ~Window();
    bool Init();
    void Close();
    void Run();

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

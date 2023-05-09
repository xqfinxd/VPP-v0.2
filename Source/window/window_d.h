#pragma once

#include <string>

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

struct Window_D {
    bool init();
    void quit();

    bool runningFlag = false;
    uint32_t frameDuration = 0;
    SDL_Window* window = nullptr;
    glm::ivec2 size{ 0,0 };
    std::string title{};
};

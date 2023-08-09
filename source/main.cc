#include <iostream>

#include "VPP/VPP.h"

#include <SDL2/SDL.h>

int main(int argc, char** argv) {
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_Window* window =
      SDL_CreateWindow("VPP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
  VPP::InitDevice(window);

  bool running = true;
  int fps = 60;
  SDL_Event _e;
  while (true) {
    auto startTick = SDL_GetTicks();
    while (SDL_PollEvent(&_e)) {
      if (_e.type == SDL_QUIT) {
        running = false;
        break;
      }
    }
    if (!running) break;


    int deltaTick = SDL_GetTicks() - startTick;
    if (deltaTick < 1000 / fps) SDL_Delay(1000 / fps - deltaTick);
  }
  
  VPP::QuitDevice();

  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
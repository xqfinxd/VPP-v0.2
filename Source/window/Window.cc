#include "window.h"

#include "SDL2/SDL_vulkan.h"

namespace vpp {

WindowBase::~WindowBase() {
  if (window_) SDL_DestroyWindow(window_);
}

bool WindowBase::InitWindow() {
  return false;
}

void WindowBase::FlushEvents() {
  if (tags_.test(Tag::eClosed)) {
    SDL_Event quitEvent;
    quitEvent.type = SDL_QUIT;
    quitEvent.quit.timestamp = SDL_GetTicks();
    SDL_PushEvent(&quitEvent);
    tags_.reset(Tag::eClosed);
  }

  if (tags_.test(Tag::eSizeChanged)) {
    SDL_SetWindowSize(window_, size_.w, size_.h);
    tags_.reset(Tag::eSizeChanged);
  }

}

WindowBase::Size WindowBase::GetPixelSize() const {
  Size pixelSize;
  if (flags_ & SDL_WINDOW_VULKAN) {
    SDL_Vulkan_GetDrawableSize(window_, &pixelSize.w, &pixelSize.h);
  } else if (flags_ & SDL_WINDOW_OPENGL) {
    SDL_GL_GetDrawableSize(window_, &pixelSize.w, &pixelSize.h);
  } else {
    if (auto renderer = SDL_GetRenderer(window_))
      SDL_GetRendererOutputSize(renderer, &pixelSize.w, &pixelSize.h);
    else
      SDL_GetWindowSize(window_, &pixelSize.w, &pixelSize.h);
  }

  return pixelSize;
}

}
#pragma once

#include <SDL2/SDL.h>
#include <bitset>

// clang-format on

namespace vpp {

class WindowBase {
public:
  virtual ~WindowBase();
  void SetWidth(int width) {
    size_.w = width;
    tags_.set(Tag::eSizeChanged);
  }
  void SetHeight(int height) {
    size_.h = height;
    tags_.set(Tag::eSizeChanged);
  }
  void SetSize(int width, int height) {
    size_.w = width;
    size_.h = height;
    tags_.set(Tag::eSizeChanged);
  }
  void Close() {
    tags_.set(Tag::eClosed);
  }

protected:
  enum Tag : uint32_t {
    eClosed,
    eSizeChanged,
    MaxCount,
  };
  struct Size {
    int w = 0, h = 0;
  };

  WindowBase() {}
  bool InitWindow();
  void FlushEvents();

  Size GetSize() const {
    return size_;
  }
  Size GetPixelSize() const;
  
  virtual void OnResize(int width, int height) = 0;

protected:
  SDL_Window*                 window_ = nullptr;
  std::bitset<Tag::MaxCount>  tags_;
  Size                        size_;
  uint32_t                    flags_ = 0;
};

} // namespace vpp

// clang-format on
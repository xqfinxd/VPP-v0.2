#pragma once

#ifdef VPPIMAGE_EXPORTS
#define IMAGE_API __declspec(dllexport)
#else
#define IMAGE_API __declspec(dllimport)
#endif

#include <cstdint>

namespace VPP {
namespace Image {

class IMAGE_API Reader {
public:
  ~Reader();
  bool Load(const char* file, uint32_t channel);

  const void* pixel() const { return pixel_; }
  uint32_t width() const { return width_; }
  uint32_t height() const { return height_; }
  uint32_t channel() const { return channel_; }
  size_t size() const { return width_ * height_ * channel_ * sizeof(uint8_t); }

private:
  void* pixel_ = nullptr;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  uint32_t channel_ = 0;
};

} // namespace Image
} // namespace VPP

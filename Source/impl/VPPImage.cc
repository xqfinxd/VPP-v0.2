#include "VPPImage.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

namespace VPP {
namespace stb {

Reader::~Reader() {
  if (pixel_) {
    stbi_image_free(pixel_);
  }
}

bool Reader::Load(const char* file, uint32_t channel) {
  if (pixel_) {
    stbi_image_free(pixel_);
  }
  // stbi_set_flip_vertically_on_load(true);
  pixel_ = (void*)stbi_load(file, (int*)&width_, (int*)&height_,
                            (int*)&channel_, (int)channel);
  // stbi_set_flip_vertically_on_load(false);
  return pixel_ != nullptr;
}

} // namespace stb
} // namespace VPP
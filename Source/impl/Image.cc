#include "Image.h"

namespace VPP {
namespace impl {
CombinedImageSampler::CombinedImageSampler() : DeviceResource() {
}

CombinedImageSampler::~CombinedImageSampler() {
  if (memory_) {
    device().free(memory_);
  }
  if (image_) {
    device().destroy(image_);
  }
  if (view_) {
    device().destroy(view_);
  }
  if (memory_) {
    device().free(memory_);
  }
}

bool CombinedImageSampler::Init(vk::Format format, uint32_t width,
                                uint32_t height, void* data, size_t size) {
  auto imageCI = vk::ImageCreateInfo()
                     .setImageType(vk::ImageType::e2D)
                     .setFormat(vk::Format::eR32G32B32A32Sfloat)
                     .setExtent({width, height, 1})
                     .setMipLevels(1)
                     .setArrayLayers(1)
                     .setSamples(vk::SampleCountFlagBits::e1)
                     .setTiling(vk::ImageTiling::eLinear)
                     .setUsage(vk::ImageUsageFlagBits::eSampled)
                     .setSharingMode(vk::SharingMode::eExclusive)
                     .setQueueFamilyIndexCount(0)
                     .setPQueueFamilyIndices(nullptr)
                     .setInitialLayout(vk::ImageLayout::ePreinitialized);

  auto result = device().createImage(&imageCI, nullptr, &image_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  vk::MemoryRequirements req{};
  device().getImageMemoryRequirements(image_, &req);
  memory_ = CreateMemory(req, vk::MemoryPropertyFlagBits::eHostVisible |
                                  vk::MemoryPropertyFlagBits::eHostCoherent);
  if (!memory_) {
    return false;
  }

  auto memSize = std::min(req.size, size);
  auto ptr = device().mapMemory(memory_, 0, memSize);
  if (!ptr) {
    return false;
  }

  memcpy(ptr, data, size);
  device().unmapMemory(memory_);

  device().bindImageMemory(image_, memory_, 0);

  auto imageViewCI = vk::ImageViewCreateInfo()
                         .setImage(image_)
                         .setViewType(vk::ImageViewType::e2D)
                         .setFormat(vk::Format::eR32G32B32A32Sfloat)
                         .setSubresourceRange(vk::ImageSubresourceRange(
                             vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

  result = device().createImageView(&imageViewCI, nullptr, &view_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  auto samplerInfo = vk::SamplerCreateInfo()
                         .setMagFilter(vk::Filter::eNearest)
                         .setMinFilter(vk::Filter::eNearest)
                         .setMipmapMode(vk::SamplerMipmapMode::eNearest)
                         .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                         .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                         .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                         .setMipLodBias(0.0f)
                         .setAnisotropyEnable(VK_FALSE)
                         .setMaxAnisotropy(1)
                         .setCompareEnable(VK_FALSE)
                         .setCompareOp(vk::CompareOp::eNever)
                         .setMinLod(0.0f)
                         .setMaxLod(0.0f)
                         .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
                         .setUnnormalizedCoordinates(VK_FALSE);
  result = device().createSampler(&samplerInfo, nullptr, &sampler_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  return true;
}
} // namespace impl
} // namespace VPP
#include "Image.h"

namespace VPP {
namespace impl {
SamplerTexture::SamplerTexture(Device* parent) : DeviceResource(parent) {}

SamplerTexture::~SamplerTexture() {
  if (sampler_) {
    device().destroy(sampler_);
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

bool SamplerTexture::SetImage2D(vk::Format format, uint32_t width,
                                uint32_t height, uint32_t channel,
                                const void* data) {
  width_ = width;
  height_ = height;
  format_ = format;

  size_t size = width_ * height_ * channel;
  
  auto imageCI = vk::ImageCreateInfo()
                     .setImageType(vk::ImageType::e2D)
                     .setFormat(format_)
                     .setExtent({width, height, 1})
                     .setMipLevels(1)
                     .setArrayLayers(1)
                     .setSamples(vk::SampleCountFlagBits::e1)
                     .setTiling(vk::ImageTiling::eOptimal)
                     .setUsage(vk::ImageUsageFlagBits::eSampled |
                               vk::ImageUsageFlagBits::eTransferDst)
                     .setSharingMode(vk::SharingMode::eExclusive)
                     .setQueueFamilyIndexCount(0)
                     .setPQueueFamilyIndices(nullptr)
                     .setInitialLayout(vk::ImageLayout::eUndefined);

  image_ = device().createImage(imageCI);
  if (!image_) {
    return false;
  }

  memory_ = CreateMemory(device().getImageMemoryRequirements(image_),
                         vk::MemoryPropertyFlagBits::eDeviceLocal);
  if (!memory_) {
    return false;
  }
  device().bindImageMemory(image_, memory_, 0);

  auto stageBuffer = CreateStageBuffer(data, size);
  if (!stageBuffer->CopyToImage(image_, width_, height_, channel)) {
    return false;
  }

  auto imageViewCI = vk::ImageViewCreateInfo()
                         .setImage(image_)
                         .setViewType(vk::ImageViewType::e2D)
                         .setFormat(format_)
                         .setSubresourceRange(vk::ImageSubresourceRange(
                             vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

  view_ = device().createImageView(imageViewCI);
  if (!view_) {
    return false;
  }

  auto samplerInfo = vk::SamplerCreateInfo()
                         .setMagFilter(vk::Filter::eLinear)
                         .setMinFilter(vk::Filter::eLinear)
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
  sampler_ = device().createSampler(samplerInfo);
  if (!sampler_) {
    return false;
  }

  return true;
}
} // namespace impl
} // namespace VPP
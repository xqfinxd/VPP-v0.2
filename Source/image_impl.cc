#include "image_impl.h"

namespace VPP {

namespace impl {

Image::Image() {}

Image::~Image() {
  if (image_) {
    Renderer::GetMe().device.destroy(image_);
  }
  if (view_) {
    Renderer::GetMe().device.destroy(view_);
  }
  if (memory_) {
    Renderer::GetMe().device.free(memory_);
  }
}

bool Image::Init(vk::Format format, uint32_t width, uint32_t height) {
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

  auto result = Renderer::GetMe().device.createImage(&imageCI, nullptr, &image_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  vk::MemoryRequirements req{};
  Renderer::GetMe().device.getImageMemoryRequirements(image_, &req);

  vk::MemoryAllocateInfo memoryAI = vk::MemoryAllocateInfo();
  memoryAI.setAllocationSize(req.size);
  memoryAI.setMemoryTypeIndex(0);
  auto memType = vk::MemoryPropertyFlagBits::eHostVisible |
                 vk::MemoryPropertyFlagBits::eHostCoherent;
  auto pass = Renderer::GetMe().FindMemoryType(req.memoryTypeBits, memType,
                                               memoryAI.memoryTypeIndex);
  if (result != vk::Result::eSuccess) {
    return false;
  }
  result = Renderer::GetMe().device.allocateMemory(&memoryAI, nullptr, &memory_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  Renderer::GetMe().device.bindImageMemory(image_, memory_, 0);

  memory_size_ = req.size;

  auto const imageViewCI =
      vk::ImageViewCreateInfo()
          .setImage(image_)
          .setViewType(vk::ImageViewType::e2D)
          .setFormat(vk::Format::eR32G32B32A32Sfloat)
          .setSubresourceRange(vk::ImageSubresourceRange(
              vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

  result = Renderer::GetMe().device.createImageView(&imageViewCI, nullptr, &view_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  return true;
}

bool Image::SetData(void* data, size_t size) {
  vk::DeviceSize mapSize = std::min(memory_size_, size);

  auto ptr = Renderer::GetMe().device.mapMemory(memory_, 0, mapSize);
  if (!ptr) {
    return false;
  }
  memcpy(ptr, data, size);
  Renderer::GetMe().device.unmapMemory(memory_);

  return true;
}

Sampler::Sampler() {}

Sampler::~Sampler() {
  auto& device = Renderer::GetMe().Renderer::GetMe().device;

  if (sampler_) {
  }
}

bool Sampler::Init() {
  vk::SamplerCreateInfo samplerInfo =
      vk::SamplerCreateInfo()
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
  auto result = Renderer::GetMe().device.createSampler(&samplerInfo, nullptr, &sampler_);
  if (result != vk::Result::eSuccess) {
    return false;
  }
  return true;
}

}  // namespace impl

}  // namespace VPP
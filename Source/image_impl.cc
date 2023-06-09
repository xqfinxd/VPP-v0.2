#include "image_impl.h"

namespace VPP {
namespace impl {
Image::Image() {}

Image::~Image() {
  auto& device = Renderer::Ref().device;

  if (image_) {
    device.destroy(image_);
  }
  if (view_) {
    device.destroy(view_);
  }
  if (memory_) {
    device.free(memory_);
  }
}

bool Image::Init(vk::Format format, uint32_t width, uint32_t height) {
  auto& rnd = Renderer::Ref();

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

  auto result = rnd.device.createImage(&imageCI, nullptr, &image_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  vk::MemoryRequirements req{};
  rnd.device.getImageMemoryRequirements(image_, &req);

  vk::MemoryAllocateInfo memoryAI = vk::MemoryAllocateInfo();
  memoryAI.setAllocationSize(req.size);
  memoryAI.setMemoryTypeIndex(0);
  auto memType = vk::MemoryPropertyFlagBits::eHostVisible |
                 vk::MemoryPropertyFlagBits::eHostCoherent;
  auto pass =
      rnd.FindMemoryType(req.memoryTypeBits, memType, memoryAI.memoryTypeIndex);
  if (result != vk::Result::eSuccess) {
    return false;
  }
  result = rnd.device.allocateMemory(&memoryAI, nullptr, &memory_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  rnd.device.bindImageMemory(image_, memory_, 0);

  memory_size_ = req.size;

  auto const imageViewCI =
      vk::ImageViewCreateInfo()
          .setImage(image_)
          .setViewType(vk::ImageViewType::e2D)
          .setFormat(vk::Format::eR32G32B32A32Sfloat)
          .setSubresourceRange(vk::ImageSubresourceRange(
              vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

  result = rnd.device.createImageView(&imageViewCI, nullptr, &view_);
  if (result != vk::Result::eSuccess) {
    return false;
  }

  return true;
}

bool Image::SetData(void* data, size_t size) {
  auto& device = Renderer::Ref().device;

  vk::DeviceSize mapSize = std::min(memory_size_, size);

  auto ptr = device.mapMemory(memory_, 0, mapSize);
  if (!ptr) {
    return false;
  }
  memcpy(ptr, data, size);
  device.unmapMemory(memory_);

  return true;
}

Sampler::Sampler() {}

Sampler::~Sampler() {
  auto& device = Renderer::Ref().device;

  if (sampler_) {
    device.destroy(sampler_);
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
  auto result =
      Renderer::Ref().device.createSampler(&samplerInfo, nullptr, &sampler_);
  if (result != vk::Result::eSuccess) {
    return false;
  }
  return true;
}
}  // namespace impl
}  // namespace VPP
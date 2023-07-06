#include "DrawCmd.h"

#include "Pipeline.h"

namespace VPP {

namespace impl {

void DrawParam::Call(const vk::CommandBuffer& buf,
                     const vk::Framebuffer& framebuffer,
                     const vk::RenderPass& renderpass) const {
  if (!pipeline_ || !vertices_ || !buf) {
    return;
  }
  auto beginInfo = vk::CommandBufferBeginInfo();
  buf.begin(beginInfo);

  const auto rpBegin =
      vk::RenderPassBeginInfo()
          .setRenderPass(renderpass)
          .setFramebuffer(framebuffer)
          .setRenderArea(vk::Rect2D(vk::Offset2D{0, 0}, surface_extent()))
          .setClearValues(clear_values_);
  buf.beginRenderPass(rpBegin, vk::SubpassContents::eInline);

  auto extent = surface_extent();
  std::vector<vk::Viewport> viewports = {vk::Viewport()
                                             .setWidth((float)extent.width)
                                             .setHeight((float)extent.height)
                                             .setMinDepth((float)0.0f)
                                             .setMaxDepth((float)1.0f)};
  buf.setViewport(0, viewports);
  std::vector<vk::Rect2D> scissors = {vk::Rect2D{vk::Offset2D(0, 0), extent}};
  buf.setScissor(0, scissors);

  pipeline_->BindCmd(buf);
  vertices_->BindCmd(buf);
  vertices_->DrawAtCmd(buf);

  buf.endRenderPass();
  buf.end();
}

bool DrawParam::BindTexture(uint32_t slot, uint32_t set, uint32_t binding) {
  auto iter =
      std::find_if(sampler_textures_.begin(), sampler_textures_.end(),
                   [slot](const std::pair<uint32_t, const SamplerTexture*>& e) {
                     return e.first == slot;
                   });
  if (iter == sampler_textures_.end()) {
    return false;
  }
  auto imageInfo = vk::DescriptorImageInfo()
                       .setImageView(iter->second->view())
                       .setSampler(iter->second->sampler())
                       .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

  auto write = vk::WriteDescriptorSet()
                   .setDescriptorCount(1)
                   .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                   .setDstSet(pipeline_->descriptor_sets_[set])
                   .setDstBinding(binding)
                   .setPImageInfo(&imageInfo);
  device().updateDescriptorSets(1, &write, 0, nullptr);
  return true;
}

bool DrawParam::BindUniform(uint32_t slot, uint32_t set, uint32_t binding) {
    auto iter = std::find_if(
        uniform_buffers_.begin(), uniform_buffers_.end(),
        [slot](const std::pair<uint32_t, const UniformBuffer*>& e) {
        return e.first == slot;
    });
    if (iter == uniform_buffers_.end()) {
        return false;
    }
    auto bufferInfo = vk::DescriptorBufferInfo()
        .setBuffer(iter->second->buffer())
        .setOffset(0)
        .setRange(iter->second->size());

    auto write = vk::WriteDescriptorSet()
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setDstSet(pipeline_->descriptor_sets_[set])
        .setDstBinding(binding)
        .setPBufferInfo(&bufferInfo);
    device().updateDescriptorSets(1, &write, 0, nullptr);
    return true;
}

} // namespace impl
} // namespace VPP

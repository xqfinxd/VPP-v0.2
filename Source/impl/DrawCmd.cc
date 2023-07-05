#include "DrawCmd.h"

#include "Pipeline.h"

namespace VPP {

namespace impl {

void DrawParam::Call(const vk::CommandBuffer& buf,
                     const vk::Framebuffer&   framebuffer,
                     const vk::RenderPass& renderpass, uint32_t subpass,
                     vk::Rect2D area, vk::Viewport viewport, vk::Rect2D scissor,
                     const VertexArray* vertices) const {
  if (!vertices || !buf) {
    return;
  }
  if (pipeline_) {
    buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);
  }
  if (!descriptor_sets_.empty()) {
    std::vector<uint32_t> offset{};
    buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipe_layout_, 0,
                           descriptor_sets_, offset);
  }

  auto beginInfo = vk::CommandBufferBeginInfo();
  buf.begin(beginInfo);

  const auto rpBegin = vk::RenderPassBeginInfo()
                           .setRenderPass(renderpass)
                           .setFramebuffer(framebuffer)
                           .setRenderArea(area)
                           .setClearValues(clear_values_);
  buf.beginRenderPass(rpBegin, vk::SubpassContents::eInline);

  std::vector<vk::Viewport> viewports = {viewport};
  buf.setViewport(0, viewports);
  std::vector<vk::Rect2D> scissors = {scissor};
  buf.setScissor(0, scissors);

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
                   .setDstSet(descriptor_sets_[set])
                   .setDstBinding(binding)
                   .setPImageInfo(&imageInfo);
  device().updateDescriptorSets(1, &write, 0, nullptr);
  return true;
}

bool DrawParam::BindBlock(uint32_t slot, uint32_t set, uint32_t binding) {
  auto iter =
      std::find_if(uniform_buffers_.begin(), uniform_buffers_.end(),
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
                   .setDstSet(descriptor_sets_[set])
                   .setDstBinding(binding)
                   .setPBufferInfo(&bufferInfo);
  device().updateDescriptorSets(1, &write, 0, nullptr);
  return true;
}

void DrawParam::UseProgram(const Program* program) {
  descriptor_pool_ = program->CreateDescriptorPool();
  descriptor_sets_ = program->AllocateDescriptorSets(descriptor_pool_);
}

} // namespace impl
} // namespace VPP

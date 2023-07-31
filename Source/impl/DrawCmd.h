#pragma once

#include <vulkan/vulkan.hpp>

#include <map>

#include "Buffer.h"
#include "Device.h"
#include "Image.h"
#include "Pipeline.h"

namespace VPP {

namespace impl {
class RenderPath : public DeviceResource {
  friend class Device;

public:
  RenderPath(Device* parent) : DeviceResource(parent) {}
  ~RenderPath() {
    if (render_pass_)
      device().destroy(render_pass_);
    for (auto& fb : frame_buffers_) {
      device().destroy(fb);
    }
  }

  const vk::RenderPass& render_pass() const { return render_pass_; }
  const vk::Framebuffer& frame_buffer(uint32_t index) const {
    return frame_buffers_[index];
  }
  const std::vector<vk::Framebuffer>& frame_buffers() const {
    return frame_buffers_;
  }
  const vk::Extent2D& extent() const { return extent_; }

protected:
  void CreateFrameBuffer(vk::Extent2D surfaceExtent,
                         uint32_t swapchainImageCount,
                         const vk::ImageView* swapchainImageViews,
                         const vk::ImageView& depthImageView) {
    std::vector<vk::Framebuffer> res;
    res.reserve(swapchainImageCount);
    res.resize(swapchainImageCount);

    vk::ImageView attachments[2];
    attachments[1] = depthImageView;

    auto frameBufferCI = vk::FramebufferCreateInfo()
                             .setRenderPass(render_pass_)
                             .setAttachmentCount(2)
                             .setPAttachments(attachments)
                             .setWidth(surfaceExtent.width)
                             .setHeight(surfaceExtent.height)
                             .setLayers(1);

    for (uint32_t i = 0; i < swapchainImageCount; i++) {
      attachments[0] = swapchainImageViews[i];
      res[i] = device().createFramebuffer(frameBufferCI);
    }
    frame_buffers_ = res;

    extent_ = surfaceExtent;
  }

  void CreateRenderPass(vk::Format colorFormat, vk::Format depthFormat) {
    vk::Result result = vk::Result::eSuccess;

    std::vector<vk::AttachmentDescription> attachments = {
        vk::AttachmentDescription()
            .setFormat(colorFormat)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR),
        vk::AttachmentDescription()
            .setFormat(depthFormat)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)};

    auto colorReference = vk::AttachmentReference().setAttachment(0).setLayout(
        vk::ImageLayout::eColorAttachmentOptimal);

    auto depthReference = vk::AttachmentReference().setAttachment(1).setLayout(
        vk::ImageLayout::eDepthStencilAttachmentOptimal);

    auto subpass = vk::SubpassDescription()
                       .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                       .setInputAttachmentCount(0)
                       .setPInputAttachments(nullptr)
                       .setColorAttachmentCount(1)
                       .setPColorAttachments(&colorReference)
                       .setPResolveAttachments(nullptr)
                       .setPDepthStencilAttachment(&depthReference)
                       .setPreserveAttachmentCount(0)
                       .setPPreserveAttachments(nullptr);

    auto renderPassCI = vk::RenderPassCreateInfo()
                            .setAttachments(attachments)
                            .setSubpassCount(1)
                            .setPSubpasses(&subpass)
                            .setDependencyCount(0)
                            .setPDependencies(nullptr);

    render_pass_ = device().createRenderPass(renderPassCI);
  }

private:
  vk::RenderPass render_pass_;
  std::vector<vk::Framebuffer> frame_buffers_;
  vk::Extent2D extent_;
};

class DrawParam : public DeviceResource {
public:
  DrawParam(Device* parent) : DeviceResource(parent) {}
  ~DrawParam();

  void SetClearValues(std::vector<vk::ClearValue>& clearValues) {
    clear_values_.swap(clearValues);
  }

  void Bind(PipelineInfo* pipeline, VertexArray* vertexArray,
            RenderPath* renderPath) {
    pipeline_info_ = pipeline;
    vertices_ = vertexArray;
    render_path_ = renderPath;

    pipeline->CreateDescriptorSets(descriptor_pool_, descriptor_sets_);

    auto vertexBindings = vertexArray->GetBindings();
    auto vertexAttrib = vertexArray->GetAttrib();

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfo =
        pipeline->shaders();
    for (auto& e : shaderStageInfo) {
      e.setPName("main");
    }

    auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
                               .setVertexAttributeDescriptions(vertexAttrib)
                               .setVertexBindingDescriptions(vertexBindings);

    auto inputAssemblyInfo =
        vk::PipelineInputAssemblyStateCreateInfo().setTopology(
            vk::PrimitiveTopology::eTriangleList);

    auto extent = render_path_->extent();
    std::vector<vk::Viewport> viewports = {vk::Viewport()
                                               .setWidth((float)extent.width)
                                               .setHeight((float)extent.height)
                                               .setMinDepth((float)0.0f)
                                               .setMaxDepth((float)1.0f)};
    std::vector<vk::Rect2D> scissors = {vk::Rect2D{vk::Offset2D(0, 0), extent}};
    auto viewportInfo = vk::PipelineViewportStateCreateInfo()
                            .setViewports(viewports)
                            .setScissors(scissors);

    auto rasterizationInfo = vk::PipelineRasterizationStateCreateInfo()
                                 .setDepthClampEnable(VK_FALSE)
                                 .setRasterizerDiscardEnable(VK_FALSE)
                                 .setPolygonMode(vk::PolygonMode::eFill)
                                 .setCullMode(vk::CullModeFlagBits::eNone)
                                 .setFrontFace(vk::FrontFace::eCounterClockwise)
                                 .setDepthBiasEnable(VK_FALSE)
                                 .setLineWidth(1.f);
    auto multisampleInfo = vk::PipelineMultisampleStateCreateInfo();

    auto stencilOp = vk::StencilOpState()
                         .setFailOp(vk::StencilOp::eKeep)
                         .setPassOp(vk::StencilOp::eKeep)
                         .setCompareOp(vk::CompareOp::eAlways);

    auto depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
                                .setDepthTestEnable(VK_TRUE)
                                .setDepthWriteEnable(VK_TRUE)
                                .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
                                .setDepthBoundsTestEnable(VK_FALSE)
                                .setStencilTestEnable(VK_FALSE)
                                .setFront(stencilOp)
                                .setBack(stencilOp);

    vk::PipelineColorBlendAttachmentState const colorBlendAttachments[1] = {
        vk::PipelineColorBlendAttachmentState().setColorWriteMask(
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)};

    auto colorBlendInfo = vk::PipelineColorBlendStateCreateInfo()
                              .setAttachmentCount(1)
                              .setPAttachments(colorBlendAttachments);

    std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport,
                                                   vk::DynamicState::eScissor};

    auto dynamicStateInfo =
        vk::PipelineDynamicStateCreateInfo().setDynamicStates(dynamicStates);

    auto pipelineCI = vk::GraphicsPipelineCreateInfo()
                          .setStages(shaderStageInfo)
                          .setPVertexInputState(&vertexInputInfo)
                          .setPInputAssemblyState(&inputAssemblyInfo)
                          .setPViewportState(&viewportInfo)
                          .setPRasterizationState(&rasterizationInfo)
                          .setPMultisampleState(&multisampleInfo)
                          .setPDepthStencilState(&depthStencilInfo)
                          .setPColorBlendState(&colorBlendInfo)
                          .setPDynamicState(&dynamicStateInfo)
                          .setLayout(pipeline->pipeline_layout())
                          .setRenderPass(renderPath->render_pass());

    auto result = device().createGraphicsPipelines(
        VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline_);
  }

  void SetTexture(uint32_t binding, SamplerTexture* texture) {
    textures_[binding] = texture;
    auto imageInfo =
        vk::DescriptorImageInfo()
            .setImageView(texture->view())
            .setSampler(texture->sampler())
            .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    auto write =
        vk::WriteDescriptorSet()
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDstSet(descriptor_sets_[1])
            .setDstBinding(binding)
            .setPImageInfo(&imageInfo);
    device().updateDescriptorSets(1, &write, 0, nullptr);
  }
  void SetUniform(uint32_t binding, UniformBuffer* ubo) {
    auto bufferInfo = vk::DescriptorBufferInfo()
                          .setBuffer(ubo->buffer())
                          .setOffset(0)
                          .setRange(ubo->size());

    auto write = vk::WriteDescriptorSet()
                     .setDescriptorCount(1)
                     .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                     .setDstSet(descriptor_sets_[0])
                     .setDstBinding(binding)
                     .setPBufferInfo(&bufferInfo);
    device().updateDescriptorSets(1, &write, 0, nullptr);
  }

  void Render(const vk::CommandBuffer& buf, uint32_t currentBuffer) {
    auto beginInfo = vk::CommandBufferBeginInfo();
    buf.begin(beginInfo);

    auto extent = render_path_->extent();

    const auto rpBegin =
        vk::RenderPassBeginInfo()
            .setRenderPass(render_path_->render_pass())
            .setFramebuffer(render_path_->frame_buffer(currentBuffer))
            .setRenderArea(vk::Rect2D(vk::Offset2D{0, 0}, extent))
            .setClearValues(clear_values_);
    buf.beginRenderPass(rpBegin, vk::SubpassContents::eInline);

    std::vector<vk::Viewport> viewports = {vk::Viewport()
                                               .setWidth((float)extent.width)
                                               .setHeight((float)extent.height)
                                               .setMinDepth((float)0.0f)
                                               .setMaxDepth((float)1.0f)};
    buf.setViewport(0, viewports);
    std::vector<vk::Rect2D> scissors = {vk::Rect2D{vk::Offset2D(0, 0), extent}};
    buf.setScissor(0, scissors);

    buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);
    if (!descriptor_sets_.empty()) {
      std::vector<uint32_t> offset{};
      buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                             pipeline_info_->pipeline_layout(), 0,
                             descriptor_sets_, offset);
    }
    vertices_->BindCmd(buf);
    vertices_->DrawAtCmd(buf);

    buf.endRenderPass();
    buf.end();
  }

private:
  const VertexArray* vertices_ = nullptr;
  const PipelineInfo* pipeline_info_ = nullptr;
  const RenderPath* render_path_ = nullptr;

  std::map<uint32_t, const SamplerTexture*> textures_{};
  std::map<uint32_t, const UniformBuffer*> uniform_{};

  std::vector<vk::ClearValue> clear_values_{};
  vk::DescriptorPool descriptor_pool_;
  std::vector<vk::DescriptorSet> descriptor_sets_;
  vk::Pipeline pipeline_;
  vk::PipelineCache pipeline_cache_;
};

} // namespace impl

} // namespace VPP

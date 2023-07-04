#include "RenderPass.h"

namespace VPP {

namespace impl {

	
GeneralRenderPass::GeneralRenderPass(Device* parent) : DeviceResource(parent) {}

GeneralRenderPass::~GeneralRenderPass() {}

const vk::Framebuffer& GeneralRenderPass::GetFramebuffer(uint32_t index) const {
  return framebuffers_[index];
}

vk::RenderPass
GeneralRenderPass::CreateRenderPass(vk::Format colorFormat,
                                    vk::Format depthFormat) const {
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

  return device().createRenderPass(renderPassCI);
}

bool GeneralRenderPass::InitFramebuffers(vk::Extent2D   extent,
                                         uint32_t       imageCount,
                                         vk::ImageView* colorImages,
                                         vk::ImageView  depthImage) {
  framebuffers_.reserve(imageCount);
  framebuffers_.resize(imageCount);

  vk::ImageView attachments[2];
  attachments[1] = depthImage;

  auto frameBufferCI = vk::FramebufferCreateInfo()
                           .setRenderPass(renderpass_)
                           .setAttachmentCount(2)
                           .setPAttachments(attachments)
                           .setWidth(extent.width)
                           .setHeight(extent.height)
                           .setLayers(1);

  for (uint32_t i = 0; i < imageCount; ++i) {
    attachments[0] = colorImages[i];
    if(device().createFramebuffer(&frameBufferCI, nullptr, &framebuffers_[i]) != vk::Result::eSuccess) {
      return false;
    }
  }
}

std::vector<vk::CommandBuffer>
GeneralRenderPass::DrawCommand(const vk::RenderPass&  renderpass,
                               const vk::Framebuffer& framebuffer,
                               const vk::Extent2D&    area) {
  return std::vector<vk::CommandBuffer>();
}




} // namespace impl
} // namespace VPP

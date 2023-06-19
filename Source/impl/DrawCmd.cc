#include "DrawCmd.h"

namespace VPP {

namespace impl {

void DrawCmd::Call(const vk::CommandBuffer& buf,
                   const vk::Framebuffer& framebuffer,
                   const vk::RenderPass& renderpass) const {
  if (!pipeline_ || !vertices_ || !buf) {
    return;
  }
  auto beginInfo = vk::CommandBufferBeginInfo().setFlags(
      vk::CommandBufferUsageFlagBits::eSimultaneousUse);
  buf.begin(beginInfo);

  const auto rpBegin =
      vk::RenderPassBeginInfo()
          .setRenderPass(renderpass)
          .setFramebuffer(framebuffer)
          .setRenderArea(vk::Rect2D(vk::Offset2D{0, 0}, surface_extent()))
          .setClearValues(clear_values_);
  buf.beginRenderPass(rpBegin, vk::SubpassContents::eInline);

  pipeline_->BindCmd(buf);
  vertices_->BindCmd(buf);
  vertices_->DrawAtCmd(buf);

  buf.endRenderPass();
  buf.end();
}

} // namespace impl
} // namespace VPP

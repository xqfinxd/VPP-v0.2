#include "Pipeline.h"

#include <map>

namespace VPP {

namespace impl {

Pipeline::Pipeline() : DeviceResource() {}

Pipeline::~Pipeline() {
  if (pipeline_) {
    device().destroy(pipeline_);
  }
  if (pipe_layout_) {
    device().destroy(pipe_layout_);
  }
  if (desc_pool_) {
    device().destroy(desc_pool_);
  }
  for (auto& e : desc_layout_) {
    if (e) {
      device().destroy(e);
    }
  }
  for (auto& e : shaders_) {
    if (e.shader) {
      device().destroy(e.shader);
    }
  }
}

bool Pipeline::SetShader(const Shader::MetaData& data) {
  std::map<uint32_t, std::vector<const Shader::Uniform*>> dataMap{};
  std::map<vk::DescriptorType, uint32_t> poolMap{};
  for (const auto& e : data.uniforms) {
    dataMap[e.set].push_back(&e);
    poolMap[e.type] += e.count;
  }

  for (const auto& e : dataMap) {
    std::vector<vk::DescriptorSetLayoutBinding> bindings{};
    bindings.reserve(e.second.size());
    for (const auto* e : e.second) {
      bindings.emplace_back(*e);
    }
    auto info = vk::DescriptorSetLayoutCreateInfo().setBindings(bindings);
    desc_layout_.emplace_back(device().createDescriptorSetLayout(info));
    if (!desc_layout_.back()) {
      return false;
    }
  }
  std::vector<vk::PushConstantRange> pushRanges{};
  for (const auto& e : data.pushes) {
    pushRanges.emplace_back(e);
  }
  auto layoutCI = vk::PipelineLayoutCreateInfo()
                      .setSetLayouts(desc_layout_)
                      .setPushConstantRanges(pushRanges);
  pipe_layout_ = device().createPipelineLayout(layoutCI);
  if (!pipe_layout_) {
    return false;
  }

  if (!poolMap.empty()) {
    std::vector<vk::DescriptorPoolSize> poolSizes{};
    for (const auto& e : poolMap) {
      poolSizes.emplace_back(
          vk::DescriptorPoolSize().setType(e.first).setDescriptorCount(
              e.second));
    }
    auto poolCI = vk::DescriptorPoolCreateInfo()
                      .setMaxSets((uint32_t)dataMap.size())
                      .setPoolSizes(poolSizes);
    desc_pool_ = device().createDescriptorPool(poolCI);
    if (!desc_pool_) {
      return false;
    }

    auto descAI = vk::DescriptorSetAllocateInfo()
                      .setDescriptorPool(desc_pool_)
                      .setSetLayouts(desc_layout_);
    desc_set_.resize(dataMap.size());
    if (device().allocateDescriptorSets(&descAI, desc_set_.data()) !=
        vk::Result::eSuccess) {
      return false;
    }
  }

  for (const auto& e : data.spvs) {
    Module shader{};
    auto muduleCI = vk::ShaderModuleCreateInfo().setCode(e.data);
    shader.shader = device().createShaderModule(muduleCI);
    if (!shader.shader) {
      return false;
    }
    shader.stage = e.stage;
    shaders_.push_back(shader);
  }

  return true;
}

void Pipeline::SetVertexArray(const VertexArray& array) {
  vertex_bindings_ = array.GetBindings();
}

void Pipeline::SetVertexAttrib(uint32_t location, uint32_t binding,
                               vk::Format format, uint32_t offset) {
  vertex_attribs_.emplace_back(vk::VertexInputAttributeDescription()
                                   .setLocation(location)
                                   .setBinding(binding)
                                   .setFormat(format)
                                   .setOffset(offset));
}

bool Pipeline::Enable() {
  if (pipeline_) {
    return true;
  }

  if (shaders_.empty()) {
    return false;
  }

  std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfo{};
  for (const auto& e : shaders_) {
    shaderStageInfo.emplace_back(vk::PipelineShaderStageCreateInfo()
                                     .setStage(e.stage)
                                     .setModule(e.shader)
                                     .setPName("main"));
  }

  auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
                             .setVertexAttributeDescriptions(vertex_attribs_)
                             .setVertexBindingDescriptions(vertex_bindings_);

  auto inputAssemblyInfo =
      vk::PipelineInputAssemblyStateCreateInfo().setTopology(
          vk::PrimitiveTopology::eTriangleList);

  auto& extent = surface_extent();
  std::vector<vk::Viewport> viewports = {vk::Viewport()
                                             .setWidth((float)extent.width)
                                             .setHeight((float)extent.height)
                                             .setMinDepth((float)0.0f)
                                             .setMaxDepth((float)1.0f)};
  std::vector<vk::Rect2D> scissors = {vk::Rect2D{vk::Offset2D(0, 0), extent}};
  auto viewportInfo =
      vk::PipelineViewportStateCreateInfo().setViewports(viewports).setScissors(
          scissors);

  auto rasterizationInfo = vk::PipelineRasterizationStateCreateInfo()
                               .setDepthClampEnable(VK_FALSE)
                               .setRasterizerDiscardEnable(VK_FALSE)
                               .setPolygonMode(vk::PolygonMode::eFill)
                               .setCullMode(vk::CullModeFlagBits::eBack)
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
                        .setLayout(pipe_layout_)
                        .setRenderPass(render_pass());
  auto result = device().createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineCI,
                                                 nullptr, &pipeline_);
  return result == vk::Result::eSuccess;
}

void Pipeline::BindCmd(const vk::CommandBuffer& buf) const {
  buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);
  if (!desc_set_.empty()) {
    std::vector<uint32_t> offset{};
    buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipe_layout_, 0,
                           desc_set_, offset);
  }
}

} // namespace impl
} // namespace VPP

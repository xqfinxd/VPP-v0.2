#include "Pipeline.h"
#include "Buffer.h"

#include <map>

namespace VPP {

namespace impl {

PipelineInfo::PipelineInfo(Device* parent) : DeviceResource(parent) {}

PipelineInfo::~PipelineInfo() {
  if (pipeline_) {
    device().destroy(pipeline_);
  }
  if (pipe_layout_) {
    device().destroy(pipe_layout_);
  }
  for (auto& e : desc_layout_) {
    if (e) {
      device().destroy(e);
    }
  }
  for (auto& e : shaders_) {
    if (e.module) {
      device().destroy(e.module);
    }
  }
}

bool PipelineInfo::SetShader(const glsl::MetaData& data) {
  std::map<uint32_t, std::vector<const glsl::Uniform*>> dataMap{};
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
    for (const auto& e : poolMap) {
      pool_sizes_.emplace_back(
          vk::DescriptorPoolSize().setType(e.first).setDescriptorCount(
              e.second));
    }
  }

  for (const auto& e : data.spvs) {
    vk::PipelineShaderStageCreateInfo shader{};
    auto muduleCI = vk::ShaderModuleCreateInfo().setCode(e.data);
    shader.module = device().createShaderModule(muduleCI);
    if (!shader.module) {
      return false;
    }
    shader.stage = e.stage;
    shaders_.push_back(shader);
  }

  return true;
}

} // namespace impl
} // namespace VPP

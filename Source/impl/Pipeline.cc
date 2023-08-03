#include "Pipeline.h"
#include "Buffer.h"

#include "VPPShader.h"

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

bool PipelineInfo::SetShader(const ShaderInterpreter* data) {
  std::map<vk::DescriptorType, uint32_t> poolMap{};
  
  for (uint32_t i = 0; i < data->GetDescriptorSetCount(); i++) {
    auto descriptorBindings = data->GetDescriptorSetBindings(i);

    for (const auto& e : descriptorBindings) {
      poolMap[e.descriptorType] += e.descriptorCount;
    }

    auto info =
        vk::DescriptorSetLayoutCreateInfo().setBindings(descriptorBindings);
    desc_layout_.emplace_back(device().createDescriptorSetLayout(info));
    if (!desc_layout_.back()) {
      return false;
    }
  }

  std::vector<vk::PushConstantRange> pushRanges{};
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

  auto stages = data->GetAllStages();
  for (const auto& stage : stages) {
    vk::PipelineShaderStageCreateInfo shader{};
    auto spv = data->GetStageSpirv(stage);
    auto muduleCI = vk::ShaderModuleCreateInfo().setCode(spv);
    shader.module = device().createShaderModule(muduleCI);
    if (!shader.module) {
      return false;
    }
    shader.stage = stage;
    shaders_.push_back(shader);
  }

  return true;
}

} // namespace impl
} // namespace VPP

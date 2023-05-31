#include "shader_impl.h"

#include "renderer_impl.h"
#include "shader_data.h"
#include "shader_loader.h"

namespace VPP {

namespace impl {

Shader::Shader() {}

Shader::~Shader() {
  auto& device = Renderer::GetMe().device();

  for (auto& shader : shader_infos_) {
    if (shader.shader) {
      device.destroy(shader.shader);
    }
  }

  for (auto& dsl : desc_layouts_) {
    if (dsl) {
      device.destroy(dsl);
    }
  }

  if (pipe_layout_) {
    device.destroy(pipe_layout_);
  }

  if (desc_pool_) {
    device.destroy(desc_pool_);
  }
}

void Shader::Load(std::vector<const char*> files) {
  auto* loader = LoadShader(files);
  if (!loader) {
    return;
  }
  auto* data = GetShaderData(loader);
  if (!data) {
    return;
  }

  auto& device = Renderer::GetMe().device();

  {  // descriptor set layout
    auto& layoutSets = data->layout_sets();
    desc_layouts_.resize(layoutSets.size());
    for (size_t i = 0; i < layoutSets.size(); ++i) {
      auto dslCI = vk::DescriptorSetLayoutCreateInfo().setBindings(
          layoutSets[i].bindings);

      desc_layouts_[i] = device.createDescriptorSetLayout(dslCI);
    }
  }

  {  // descriptor pool and set
    auto& layoutSets = data->layout_sets();

    std::vector<vk::DescriptorPoolSize> poolSizes{};
    for (const auto& ls : layoutSets) {
      for (const auto& lb : ls.bindings) {
        auto dstType = lb.descriptorType;

        auto it = std::find_if(poolSizes.begin(), poolSizes.end(),
                               [dstType](const vk::DescriptorPoolSize& ps) {
                                 return ps.type == dstType;
                               });

        if (it == poolSizes.end()) {
          poolSizes.push_back(
              vk::DescriptorPoolSize().setType(dstType).setDescriptorCount(
                  lb.descriptorCount));
        } else {
          it->descriptorCount += lb.descriptorCount;
        }
      }
    }

    auto poolCI =
        vk::DescriptorPoolCreateInfo().setPoolSizes(poolSizes).setMaxSets(
            layoutSets.size());
    desc_pool_ = device.createDescriptorPool(poolCI);

    if (desc_pool_) {
      auto dsAI = vk::DescriptorSetAllocateInfo()
                      .setDescriptorPool(desc_pool_)
                      .setSetLayouts(desc_layouts_);
      desc_sets_ = device.allocateDescriptorSets(dsAI);
    }

    auto plCI = vk::PipelineLayoutCreateInfo()
                    .setSetLayouts(desc_layouts_)
                    .setPushConstantRanges(data->push_constants());
    pipe_layout_ = device.createPipelineLayout(plCI);
  }

  {  // shader module
    shader_infos_.clear();
    for (const auto& spv : data->spv_datas()) {
      StageInfo info{};
      auto      smCI = vk::ShaderModuleCreateInfo().setCode(spv.data);
      info.shader = device.createShaderModule(smCI);
      info.stage = spv.stage;
      shader_infos_.push_back(info);
    }
  }

  DestroyShader(loader);
}

}  // namespace impl

}  // namespace VPP

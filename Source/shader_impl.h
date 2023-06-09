#pragma once

#include <vulkan/vulkan.hpp>

namespace VPP {
namespace impl {
class Shader {
 public:
  Shader();
  ~Shader();

  void Load(std::vector<const char*> files);

 private:
  struct StageInfo {
    vk::ShaderStageFlagBits stage = (vk::ShaderStageFlagBits)~0;
    vk::ShaderModule        shader{};
  };

  std::vector<StageInfo>               shader_infos_{};
  vk::DescriptorPool                   desc_pool_{};
  vk::PipelineLayout                   pipe_layout_{};
  std::vector<vk::DescriptorSetLayout> desc_layouts_{};
  std::vector<vk::DescriptorSet>       desc_sets_{};
};
}  // namespace impl
}  // namespace VPP

#pragma once

#include <vulkan/vulkan.hpp>s

namespace VPP {

namespace impl {

class Pipeline {
 public:
  Pipeline();
  ~Pipeline();

 private:
  vk::Pipeline pipeline_{};
  vk::PipelineCache cache_;
  vk::PipelineLayout pipe_layout_{};
  std::vector<vk::DescriptorSetLayout> desc_layout_{};
  std::vector<vk::PipelineShaderStageCreateInfo> shaders_{};
};

}  // namespace impl

}  // namespace VPP

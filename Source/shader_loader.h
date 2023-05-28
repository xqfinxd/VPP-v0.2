#pragma once

#include <map>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace VPP {

namespace impl {

typedef vk::DescriptorSetLayoutBinding LayoutBinding;
typedef vk::PushConstantRange          PushConstant;
typedef std::vector<uint32_t>          SpvData;
struct LayoutSet {
  uint32_t                                    set_num{};
  std::vector<vk::DescriptorSetLayoutBinding> bindings{};
};

struct ShaderData {
  std::vector<LayoutSet>                     layout_sets{};
  std::vector<PushConstant>                  push_constants{};
  std::map<vk::ShaderStageFlagBits, SpvData> spvs{};
};

ShaderData LoadShader(std::map<vk::ShaderStageFlagBits, const char*> files);

}  // namespace impl

}  // namespace VPP

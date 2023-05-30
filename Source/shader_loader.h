#pragma once

#include <vector>
#include <memory>
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
  std::vector<uint32_t>                      input_locations{};
  std::map<vk::ShaderStageFlagBits, SpvData> spvs{};

  void Clear() {
      layout_sets.clear();
      push_constants.clear();
      spvs.clear();
  }

  bool Empty() const {
      return spvs.empty();
  }
};

ShaderData LoadShader(std::vector<const char*> files);

}  // namespace impl

}  // namespace VPP

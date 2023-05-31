#pragma once

#include <map>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace VPP {

namespace impl {

class ShaderData {
 public:
  struct LayoutSet {
    using LB = vk::DescriptorSetLayoutBinding;
    uint32_t        set_num{};
    std::vector<LB> bindings{};
  };

  struct SpvData {
    using Stage = vk::ShaderStageFlagBits;
    using Data = std::vector<uint32_t>;
    Stage stage;
    Data  data{};
  };

  using PushConstant = vk::PushConstantRange;

 public:
  void Clear() {
    layout_sets.clear();
    push_constants.clear();
    spv_datas.clear();
  }

  bool Empty() const {
    return spv_datas.empty();
  }

  void AddBinding(uint32_t setNum, vk::DescriptorSetLayoutBinding&& binding);
  void AddPushConstant(vk::PushConstantRange&& pushConst);
  void AddLocation(uint32_t location);
  void AddSpvData(vk::ShaderStageFlagBits stage, std::vector<uint32_t>&& data);

 private:
  std::vector<LayoutSet>    layout_sets{};
  std::vector<PushConstant> push_constants{};
  std::vector<uint32_t>     locations{};
  std::vector<SpvData>      spv_datas{};
};

}  // namespace impl

}  // namespace VPP

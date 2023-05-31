#pragma once

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
    Stage stage = (Stage)~0;
    Data  data{};
  };

  using PushConstant = vk::PushConstantRange;

 public:
  void Clear() {
    layout_sets.clear();
    push_constants.clear();
    locations.clear();
    spv_datas.clear();
  }

  bool Empty() const {
    return spv_datas.empty();
  }

  void Swap(ShaderData& other) {
    layout_sets.swap(other.layout_sets);
    push_constants.swap(other.push_constants);
    locations.swap(other.locations);
    spv_datas.swap(other.spv_datas);
  }

  void Copy(const ShaderData& other) {
    layout_sets = other.layout_sets;
    push_constants = other.push_constants;
    locations = other.locations;
    spv_datas = other.spv_datas;
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

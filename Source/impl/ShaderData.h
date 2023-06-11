#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

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
    layout_sets_.clear();
    push_constants_.clear();
    locations_.clear();
    spv_datas_.clear();
  }

  bool Empty() const {
    return spv_datas_.empty();
  }

  void Swap(ShaderData& other) {
    layout_sets_.swap(other.layout_sets_);
    push_constants_.swap(other.push_constants_);
    locations_.swap(other.locations_);
    spv_datas_.swap(other.spv_datas_);
  }

  void Copy(const ShaderData& other) {
    layout_sets_ = other.layout_sets_;
    push_constants_ = other.push_constants_;
    locations_ = other.locations_;
    spv_datas_ = other.spv_datas_;
  }

  const std::vector<LayoutSet>& layout_sets() const {
    return layout_sets_;
  }
  const std::vector<PushConstant>& push_constants() const {
    return push_constants_;
  };
  const std::vector<uint32_t>& locations() const {
    return locations_;
  };
  const std::vector<SpvData>& spv_datas() const {
    return spv_datas_;
  };

  void AddBinding(uint32_t setNum, vk::DescriptorSetLayoutBinding&& binding);
  void AddPushConstant(vk::PushConstantRange&& pushConst);
  void AddLocation(uint32_t location);
  void AddSpvData(vk::ShaderStageFlagBits stage, std::vector<uint32_t>&& data);

 private:
  std::vector<LayoutSet>    layout_sets_{};
  std::vector<PushConstant> push_constants_{};
  std::vector<uint32_t>     locations_{};
  std::vector<SpvData>      spv_datas_{};
};
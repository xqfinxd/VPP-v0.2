#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace Shader {
struct Uniform {
  uint32_t set = 0;
  uint32_t binding = 0;
  vk::DescriptorType type = (vk::DescriptorType)~0;
  uint32_t count = 0;
  vk::ShaderStageFlags stages = (vk::ShaderStageFlags)0;

  operator vk::DescriptorSetLayoutBinding() const {
    return vk::DescriptorSetLayoutBinding()
        .setBinding(binding)
        .setDescriptorCount(count)
        .setDescriptorType(type)
        .setStageFlags(stages);
  }

  bool IsSame(const Uniform& other) const {
    return other.set == set && other.binding == binding && other.type == type &&
           other.stages == stages;
  }

  friend bool operator<(const Uniform& left, const Uniform& right) {
    if (left.set == right.set) {
      return left.binding < right.binding;
    } else {
      return left.set < right.set;
    }
  }
};

struct Input {
  uint32_t location = 0;
  vk::Format format = vk::Format::eUndefined;

  friend bool operator<(const Input& left, const Input& right) {
    return left.location < right.location;
  }
};

struct PushConstant {
  vk::ShaderStageFlags stages = (vk::ShaderStageFlags)0;
  uint32_t size = 0;

  operator vk::PushConstantRange() const {
    return vk::PushConstantRange().setSize(size).setOffset(0).setStageFlags(
        stages);
  }
};

struct SpvData {
  vk::ShaderStageFlagBits stage = (vk::ShaderStageFlagBits)~0;
  std::vector<uint32_t> data{};
};

struct MetaData {
  std::vector<Shader::Uniform> uniforms{};
  std::vector<Shader::PushConstant> pushes{};
  std::vector<Shader::SpvData> spvs{};
  std::vector<Shader::Input> inputs{};

  void Swap(MetaData&& other) {
    uniforms.swap(other.uniforms);
    pushes.swap(other.pushes);
    spvs.swap(other.spvs);
    inputs.swap(other.inputs);
  }
};

} // namespace Shader

#include "shader_loader.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <fstream>
#include <iostream>

#ifdef _DEBUG
#pragma comment(lib, "MachineIndependentd.lib")
#pragma comment(lib, "SPIRVd.lib")
#pragma comment(lib, "OGLCompilerd.lib")
#pragma comment(lib, "OSDependentd.lib")
#pragma comment(lib, "SPIRV-Tools-optd.lib")
#pragma comment(lib, "GenericCodeGend.lib")
#pragma comment(lib, "SPIRV-Toolsd.lib")
#else
#pragma comment(lib, "MachineIndependent.lib")
#pragma comment(lib, "SPIRV.lib")
#pragma comment(lib, "OGLCompiler.lib")
#pragma comment(lib, "OSDependent.lib")
#pragma comment(lib, "SPIRV-Tools-opt.lib")
#pragma comment(lib, "GenericCodeGen.lib")
#pragma comment(lib, "SPIRV-Tools.lib")
#endif

namespace VPP {

namespace impl {

const int                             kGlslVersion = 400;
const glslang::EShSource              kSourceLanguage = glslang::EShSourceGlsl;
const glslang::EShClient              kClient = glslang::EShClientVulkan;
const glslang::EShTargetClientVersion kClientVersion =
    glslang::EShTargetVulkan_1_1;
const glslang::EShTargetLanguage        kTargetLanguage = glslang::EShTargetSpv;
const glslang::EShTargetLanguageVersion kTargetLanguageVersion =
    glslang::EShTargetSpv_1_2;
const EShMessages kMessages =
    (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules | EShMsgAST);

void initResources(TBuiltInResource& res) {
  res.maxLights = 32;
  res.maxClipPlanes = 6;
  res.maxTextureUnits = 32;
  res.maxTextureCoords = 32;
  res.maxVertexAttribs = 64;
  res.maxVertexUniformComponents = 4096;
  res.maxVaryingFloats = 64;
  res.maxVertexTextureImageUnits = 32;
  res.maxCombinedTextureImageUnits = 80;
  res.maxTextureImageUnits = 32;
  res.maxFragmentUniformComponents = 4096;
  res.maxDrawBuffers = 32;
  res.maxVertexUniformVectors = 128;
  res.maxVaryingVectors = 8;
  res.maxFragmentUniformVectors = 16;
  res.maxVertexOutputVectors = 16;
  res.maxFragmentInputVectors = 15;
  res.minProgramTexelOffset = -8;
  res.maxProgramTexelOffset = 7;
  res.maxClipDistances = 8;
  res.maxComputeWorkGroupCountX = 65535;
  res.maxComputeWorkGroupCountY = 65535;
  res.maxComputeWorkGroupCountZ = 65535;
  res.maxComputeWorkGroupSizeX = 1024;
  res.maxComputeWorkGroupSizeY = 1024;
  res.maxComputeWorkGroupSizeZ = 64;
  res.maxComputeUniformComponents = 1024;
  res.maxComputeTextureImageUnits = 16;
  res.maxComputeImageUniforms = 8;
  res.maxComputeAtomicCounters = 8;
  res.maxComputeAtomicCounterBuffers = 1;
  res.maxVaryingComponents = 60;
  res.maxVertexOutputComponents = 64;
  res.maxGeometryInputComponents = 64;
  res.maxGeometryOutputComponents = 128;
  res.maxFragmentInputComponents = 128;
  res.maxImageUnits = 8;
  res.maxCombinedImageUnitsAndFragmentOutputs = 8;
  res.maxCombinedShaderOutputResources = 8;
  res.maxImageSamples = 0;
  res.maxVertexImageUniforms = 0;
  res.maxTessControlImageUniforms = 0;
  res.maxTessEvaluationImageUniforms = 0;
  res.maxGeometryImageUniforms = 0;
  res.maxFragmentImageUniforms = 8;
  res.maxCombinedImageUniforms = 8;
  res.maxGeometryTextureImageUnits = 16;
  res.maxGeometryOutputVertices = 256;
  res.maxGeometryTotalOutputComponents = 1024;
  res.maxGeometryUniformComponents = 1024;
  res.maxGeometryVaryingComponents = 64;
  res.maxTessControlInputComponents = 128;
  res.maxTessControlOutputComponents = 128;
  res.maxTessControlTextureImageUnits = 16;
  res.maxTessControlUniformComponents = 1024;
  res.maxTessControlTotalOutputComponents = 4096;
  res.maxTessEvaluationInputComponents = 128;
  res.maxTessEvaluationOutputComponents = 128;
  res.maxTessEvaluationTextureImageUnits = 16;
  res.maxTessEvaluationUniformComponents = 1024;
  res.maxTessPatchComponents = 120;
  res.maxPatchVertices = 32;
  res.maxTessGenLevel = 64;
  res.maxViewports = 16;
  res.maxVertexAtomicCounters = 0;
  res.maxTessControlAtomicCounters = 0;
  res.maxTessEvaluationAtomicCounters = 0;
  res.maxGeometryAtomicCounters = 0;
  res.maxFragmentAtomicCounters = 8;
  res.maxCombinedAtomicCounters = 8;
  res.maxAtomicCounterBindings = 1;
  res.maxVertexAtomicCounterBuffers = 0;
  res.maxTessControlAtomicCounterBuffers = 0;
  res.maxTessEvaluationAtomicCounterBuffers = 0;
  res.maxGeometryAtomicCounterBuffers = 0;
  res.maxFragmentAtomicCounterBuffers = 1;
  res.maxCombinedAtomicCounterBuffers = 1;
  res.maxAtomicCounterBufferSize = 16384;
  res.maxTransformFeedbackBuffers = 4;
  res.maxTransformFeedbackInterleavedComponents = 64;
  res.maxCullDistances = 8;
  res.maxCombinedClipAndCullDistances = 8;
  res.maxSamples = 4;
  res.limits.nonInductiveForLoops = 1;
  res.limits.whileLoops = 1;
  res.limits.doWhileLoops = 1;
  res.limits.generalUniformIndexing = 1;
  res.limits.generalAttributeMatrixVectorIndexing = 1;
  res.limits.generalVaryingIndexing = 1;
  res.limits.generalSamplerIndexing = 1;
  res.limits.generalVariableIndexing = 1;
  res.limits.generalConstantMatrixVectorIndexing = 1;
}

bool loadFile(const char* fn, std::string& ctx) {
  std::ifstream file(fn);  // 打开文件
  if (!file)
    return false;
  file.seekg(0, file.end);
  size_t length = file.tellg();
  ctx.resize(length + 1);
  file.seekg(0, file.beg);
  file.read(&ctx.at(0), length);
  file.close();  // 关闭文件
  return true;
}

bool compileShader(glslang::TShader* shader, const char* filename,
                   EShLanguage stage) {
  if (filename == nullptr) {
    return false;
  }
  TBuiltInResource resource;
  initResources(resource);
  std::string content{};
  if (!loadFile(filename, content)) {
    return false;
  }
  const char* const tempStrs[] = {content.c_str()};
  shader->setStrings(tempStrs, 1);
  shader->setEntryPoint("main");
  shader->setEnvInput(kSourceLanguage, static_cast<EShLanguage>(stage), kClient,
                      kGlslVersion);
  shader->setEnvClient(kClient, kClientVersion);
  shader->setEnvTarget(kTargetLanguage, kTargetLanguageVersion);
  if (!shader->parse(&resource, kGlslVersion, false, kMessages)) {
    std::cerr << shader->getInfoLog() << std::endl;
    return false;
  }
  return true;
}

vk::ShaderStageFlags stageCast(uint32_t mask) {
  vk::ShaderStageFlags                              flags{};
  const std::map<uint32_t, vk::ShaderStageFlagBits> table{
      {EShLangVertexMask, vk::ShaderStageFlagBits::eVertex},
      {EShLangTessControlMask, vk::ShaderStageFlagBits::eTessellationControl},
      {EShLangTessEvaluationMask,
       vk::ShaderStageFlagBits::eTessellationEvaluation},
      {EShLangGeometryMask, vk::ShaderStageFlagBits::eGeometry},
      {EShLangFragmentMask, vk::ShaderStageFlagBits::eFragment},
      {EShLangComputeMask, vk::ShaderStageFlagBits::eCompute},
      {EShLangRayGenMask, vk::ShaderStageFlagBits::eRaygenKHR},
      {EShLangRayGenNVMask, vk::ShaderStageFlagBits::eRaygenNV},
      {EShLangIntersectMask, vk::ShaderStageFlagBits::eIntersectionKHR},
      {EShLangIntersectNVMask, vk::ShaderStageFlagBits::eIntersectionNV},
      {EShLangAnyHitMask, vk::ShaderStageFlagBits::eAnyHitKHR},
      {EShLangAnyHitNVMask, vk::ShaderStageFlagBits::eAnyHitNV},
      {EShLangClosestHitMask, vk::ShaderStageFlagBits::eClosestHitKHR},
      {EShLangClosestHitNVMask, vk::ShaderStageFlagBits::eClosestHitNV},
      {EShLangMissMask, vk::ShaderStageFlagBits::eMissKHR},
      {EShLangMissNVMask, vk::ShaderStageFlagBits::eMissNV},
      {EShLangCallableMask, vk::ShaderStageFlagBits::eCallableKHR},
      {EShLangCallableNVMask, vk::ShaderStageFlagBits::eCallableNV},
      {EShLangTaskMask, vk::ShaderStageFlagBits::eTaskEXT},
      {EShLangTaskNVMask, vk::ShaderStageFlagBits::eTaskNV},
      {EShLangMeshMask, vk::ShaderStageFlagBits::eMeshEXT},
      {EShLangMeshNVMask, vk::ShaderStageFlagBits::eMeshNV},
  };
  for (const auto& e : table) {
    if (mask & e.first) {
      flags |= e.second;
    }
  }
  return flags;
}

bool stageCast(EShLanguage flag, vk::ShaderStageFlagBits& outStage) {
  const std::map<EShLanguage, vk::ShaderStageFlagBits> table{
      {EShLangVertex, vk::ShaderStageFlagBits::eVertex},
      {EShLangTessControl, vk::ShaderStageFlagBits::eTessellationControl},
      {EShLangTessEvaluation, vk::ShaderStageFlagBits::eTessellationEvaluation},
      {EShLangGeometry, vk::ShaderStageFlagBits::eGeometry},
      {EShLangFragment, vk::ShaderStageFlagBits::eFragment},
      {EShLangCompute, vk::ShaderStageFlagBits::eCompute},
      {EShLangRayGen, vk::ShaderStageFlagBits::eRaygenKHR},
      {EShLangRayGenNV, vk::ShaderStageFlagBits::eRaygenNV},
      {EShLangIntersect, vk::ShaderStageFlagBits::eIntersectionKHR},
      {EShLangIntersectNV, vk::ShaderStageFlagBits::eIntersectionNV},
      {EShLangAnyHit, vk::ShaderStageFlagBits::eAnyHitKHR},
      {EShLangAnyHitNV, vk::ShaderStageFlagBits::eAnyHitNV},
      {EShLangClosestHit, vk::ShaderStageFlagBits::eClosestHitKHR},
      {EShLangClosestHitNV, vk::ShaderStageFlagBits::eClosestHitNV},
      {EShLangMiss, vk::ShaderStageFlagBits::eMissKHR},
      {EShLangMissNV, vk::ShaderStageFlagBits::eMissNV},
      {EShLangCallable, vk::ShaderStageFlagBits::eCallableKHR},
      {EShLangCallableNV, vk::ShaderStageFlagBits::eCallableNV},
      {EShLangTask, vk::ShaderStageFlagBits::eTaskEXT},
      {EShLangTaskNV, vk::ShaderStageFlagBits::eTaskNV},
      {EShLangMesh, vk::ShaderStageFlagBits::eMeshEXT},
      {EShLangMeshNV, vk::ShaderStageFlagBits::eMeshNV},
  };
  auto iter = table.find(flag);
  if (iter == table.end()) {
    return false;
  }
  outStage = iter->second;
  return true;
}

EShLanguage stageCast(vk::ShaderStageFlagBits flag) {
  const std::map<vk::ShaderStageFlagBits, EShLanguage> table{
      {vk::ShaderStageFlagBits::eVertex, EShLangVertex},
      {vk::ShaderStageFlagBits::eTessellationControl, EShLangTessControl},
      {vk::ShaderStageFlagBits::eTessellationEvaluation, EShLangTessEvaluation},
      {vk::ShaderStageFlagBits::eGeometry, EShLangGeometry},
      {vk::ShaderStageFlagBits::eFragment, EShLangFragment},
      {vk::ShaderStageFlagBits::eCompute, EShLangCompute},
      {vk::ShaderStageFlagBits::eRaygenKHR, EShLangRayGen},
      {vk::ShaderStageFlagBits::eRaygenNV, EShLangRayGenNV},
      {vk::ShaderStageFlagBits::eIntersectionKHR, EShLangIntersect},
      {vk::ShaderStageFlagBits::eIntersectionNV, EShLangIntersectNV},
      {vk::ShaderStageFlagBits::eAnyHitKHR, EShLangAnyHit},
      {vk::ShaderStageFlagBits::eAnyHitNV, EShLangAnyHitNV},
      {vk::ShaderStageFlagBits::eClosestHitKHR, EShLangClosestHit},
      {vk::ShaderStageFlagBits::eClosestHitNV, EShLangClosestHitNV},
      {vk::ShaderStageFlagBits::eMissKHR, EShLangMiss},
      {vk::ShaderStageFlagBits::eMissNV, EShLangMissNV},
      {vk::ShaderStageFlagBits::eCallableKHR, EShLangCallable},
      {vk::ShaderStageFlagBits::eCallableNV, EShLangCallableNV},
      {vk::ShaderStageFlagBits::eTaskEXT, EShLangTask},
      {vk::ShaderStageFlagBits::eTaskNV, EShLangTaskNV},
      {vk::ShaderStageFlagBits::eMeshEXT, EShLangMesh},
      {vk::ShaderStageFlagBits::eMeshNV, EShLangMeshNV},
  };
  auto iter = table.find(flag);
  if (iter == table.end()) {
    throw std::exception("error stage");
  }
  return iter->second;
}

bool getType(const glslang::TType& ttype, vk::DescriptorType& type) {
  const auto& qualifier = ttype.getQualifier();
  if (ttype.getBasicType() == glslang::EbtSampler) {
    const auto& sampler = ttype.getSampler();
    if (sampler.isCombined()) {
      type = vk::DescriptorType::eCombinedImageSampler;
      return true;
    } else if (sampler.isTexture()) {
      type = vk::DescriptorType::eSampledImage;
      return true;
    } else if (sampler.isTexture()) {
      type = vk::DescriptorType::eSampledImage;
      return true;
    } else if (sampler.isImage()) {
      type = vk::DescriptorType::eStorageImage;
      return true;
    } else if (sampler.isPureSampler()) {
      type = vk::DescriptorType::eSampler;
      return true;
    } else {
      return false;
    }
  } else if (ttype.getBasicType() == glslang::EbtBlock) {
    if (qualifier.storage == glslang::EvqUniform) {
      type = vk::DescriptorType::eUniformBuffer;
      return true;
    } else if (qualifier.storage == glslang::EvqBuffer) {
      type = vk::DescriptorType::eStorageBuffer;
      return true;
    } else {
      return false;
    }
  }
  return false;
}

struct Converter {
  typedef std::vector<vk::DescriptorSetLayoutBinding> Bindings;
  std::map<uint32_t, Bindings>                        setLayouts{};
  std::vector<vk::PushConstantRange>                  pushConstants{};

  void clear() {
    setLayouts.clear();
    pushConstants.clear();
  }

  vk::PushConstantRange toPushConstant(const glslang::TObjectReflection& obj) {
    auto pushCons = vk::PushConstantRange();
    pushCons.setSize(obj.size);
    pushCons.setStageFlags(stageCast(obj.stages));
    if (obj.offset > 0) {
      pushCons.setOffset(obj.offset);
    }
    return pushCons;
  }

  vk::DescriptorSetLayoutBinding toDescriptor(
      const glslang::TObjectReflection& obj) {
    vk::DescriptorSetLayoutBinding binding{};
    auto                           ttype = obj.getType();
    if (!getType(*ttype, binding.descriptorType)) {
      throw std::exception("error descriptor type");
    }
    binding.setBinding(ttype->getQualifier().layoutBinding);
    binding.setDescriptorCount(1);
    binding.setStageFlags(stageCast(obj.stages));
    return binding;
  }

  void push(uint32_t setnum, vk::DescriptorSetLayoutBinding binding) {
    auto& vec = setLayouts[setnum];
    for (auto& e : vec) {
      if (e.binding == binding.binding) {
        if (e.stageFlags == binding.stageFlags &&
            e.descriptorType == binding.descriptorType) {
          e.descriptorCount += binding.descriptorCount;
          return;
        } else {
          throw std::exception("error descriptor info");
        }
      }
    }
    vec.push_back(binding);
  }

  void push(vk::PushConstantRange range) {
    for (auto& e : pushConstants) {
      if (e.stageFlags == range.stageFlags) {
        throw std::exception("error push constant");
      }
    }
    pushConstants.push_back(range);
  }

  void query(const glslang::TProgram& program) {
    for (int32_t i = 0; i < program.getNumUniformVariables(); i++) {
      auto& obj = program.getUniform(i);
      auto  ttype = obj.getType();
      if (ttype->getBasicType() == glslang::EbtSampler) {
        auto setNum = ttype->getQualifier().layoutSet;
        push(setNum, toDescriptor(obj));
      }
    }

    for (int i = 0; i < program.getNumUniformBlocks(); i++) {
      auto& obj = program.getUniformBlock(i);
      auto  ttype = obj.getType();
      if (ttype->getQualifier().isPushConstant()) {
        push(toPushConstant(obj));
        continue;
      }
      auto setNum = ttype->getQualifier().layoutSet;
      push(setNum, toDescriptor(obj));
    }

    for (int i = 0; i < program.getNumBufferBlocks(); i++) {
      auto& obj = program.getBufferBlock(i);
      auto  ttype = obj.getType();
      auto  setNum = ttype->getQualifier().layoutSet;
      push(setNum, toDescriptor(obj));
    }

    for (int i = 0; i < program.getNumPipeInputs(); i++) {
      auto obj = program.getPipeInput(i);
      auto ttype = obj.getType();
      auto qua = ttype->getQualifier();
      qua.storage;
    }
  }
};

struct PushPoolSize {
  typedef std::vector<vk::DescriptorPoolSize> Target;
  PushPoolSize(Target& target) : poolSizes(&target) {}
  Target* poolSizes = nullptr;
  void    operator()(const vk::DescriptorSetLayoutBinding& binding) {
    poolSizes->push_back(vk::DescriptorPoolSize()
                                .setDescriptorCount(binding.descriptorCount)
                                .setType(binding.descriptorType));
  }
};

class ShaderAnalyzer {
 public:
  ShaderAnalyzer(std::map<EShLanguage, const char*> shaders) {
    glslang::InitializeProcess();

    program = ProgramHandle(new glslang::TProgram);
    for (const auto& e : shaders) {
      auto& handle = shaderTable[e.first];
      handle.reset(new glslang::TShader(e.first));
      if (compileShader(handle.get(), e.second, e.first)) {
        program->addShader(handle.get());
      } else {
        throw std::exception("error compile shader");
      }
    }

    if (program->link(kMessages) && program->buildReflection()) {
    } else {
      throw std::exception("error link shader");
    }
  }
  ~ShaderAnalyzer() {
    glslang::FinalizeProcess();
  }

  bool getObject(const vk::Device& device, ShaderObject& object) const {
    vk::Result result = vk::Result::eSuccess;

    Converter cvt{};
    cvt.query(*program.get());

    object.setSetCount((uint32_t)cvt.setLayouts.size());
    size_t                              index = 0;
    std::vector<vk::DescriptorPoolSize> poolSizes{};

    for (const auto& e : cvt.setLayouts) {
      auto setLayoutCI =
          vk::DescriptorSetLayoutCreateInfo().setBindings(e.second);
      result = device.createDescriptorSetLayout(&setLayoutCI, nullptr,
                                                &object.setLayouts[index]);
      if (result != vk::Result::eSuccess) {
        return false;
      }

      std::for_each(e.second.begin(), e.second.end(), PushPoolSize(poolSizes));

      object.setNums[index] = e.first;
      index++;
    }

    auto poolCI =
        vk::DescriptorPoolCreateInfo().setPoolSizes(poolSizes).setMaxSets(
            object.setCount);
    result = device.createDescriptorPool(&poolCI, nullptr, &object.pool);
    if (result != vk::Result::eSuccess) {
      return false;
    }

    auto setAI = vk::DescriptorSetAllocateInfo()
                     .setDescriptorPool(object.pool)
                     .setDescriptorSetCount(object.setCount)
                     .setPSetLayouts(object.setLayouts.get());
    result = device.allocateDescriptorSets(&setAI, object.setObjects.get());
    if (result != vk::Result::eSuccess) {
      return false;
    }

    auto pipLayoutCI = vk::PipelineLayoutCreateInfo()
                           .setSetLayoutCount(object.setCount)
                           .setPSetLayouts(object.setLayouts.get())
                           .setPushConstantRanges(cvt.pushConstants);
    result =
        device.createPipelineLayout(&pipLayoutCI, nullptr, &object.pipeLayout);
    if (result != vk::Result::eSuccess) {
      return false;
    }

    for (const auto& shader : shaderTable) {
      auto intermediate = shader.second->getIntermediate();
      if (!intermediate) {
        return false;
      }

      vk::ShaderStageFlagBits stage;
      bool                    hasStage = stageCast(shader.first, stage);
      if (!hasStage) {
        return false;
      }

      auto&                 shaderModule = object.shaderModules[stage];
      std::vector<uint32_t> spv{};
      glslang::GlslangToSpv(*intermediate, spv);
      auto shaderCI = vk::ShaderModuleCreateInfo().setCode(spv);
      result = device.createShaderModule(&shaderCI, nullptr, &shaderModule);
      if (result != vk::Result::eSuccess) {
        return false;
      }
    }
    return true;
  }

 private:
  using ShaderHandle = std::unique_ptr<glslang::TShader>;
  using ProgramHandle = std::unique_ptr<glslang::TProgram>;
  std::map<EShLanguage, ShaderHandle> shaderTable{};
  ProgramHandle                       program{};
};

void ShaderObject::setSetCount(uint32_t count) {
  setCount = count;
  setLayouts = std::make_unique<vk::DescriptorSetLayout[]>(setCount);
  setObjects = std::make_unique<vk::DescriptorSet[]>(setCount);
  setNums = std::make_unique<uint32_t[]>(setCount);
}

void ShaderObject::destroy(const vk::Device& device) {
  if (pipeLayout) {
    device.destroy(pipeLayout);
  }
  if (pool) {
    device.destroy(pool);
  }

  for (auto& e : shaderModules) {
    if (e.second) {
      device.destroy(e.second);
    }
  }
  for (size_t i = 0; i < setCount; i++) {
    if (setLayouts[i]) {
      device.destroy(setLayouts[i]);
    }
  }
}

std::unique_ptr<ShaderObject> ShaderObject::createFromFiles(
    const vk::Device& device, std::map<vk::ShaderStageFlagBits, const char*> files) {
  auto shaderObject = std::make_unique<ShaderObject>();
  try {
    std::map<EShLanguage, const char*> tmpShaderFiles{};
    for (const auto& e : files) {
      tmpShaderFiles[stageCast(e.first)] = e.second;
    }
    ShaderAnalyzer analyzer(tmpShaderFiles);
    bool           success = analyzer.getObject(device, *shaderObject.get());
    if (success) {
      shaderObject->destroy(device);
      return nullptr;
    }
  } catch (std::exception e) {
    shaderObject->destroy(device);
    return nullptr;
  }

  return shaderObject;
}

}  // namespace impl

}  // namespace VPP
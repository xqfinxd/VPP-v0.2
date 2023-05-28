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

void InitResources(TBuiltInResource& res) {
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

bool LoadFile(const char* fn, std::string& ctx) {
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

bool CompileShader(glslang::TShader* shader, const char* filename,
                   EShLanguage stage) {
  if (filename == nullptr) {
    return false;
  }
  TBuiltInResource resource;
  InitResources(resource);
  std::string content{};
  if (!LoadFile(filename, content)) {
    return false;
  }
  const char* const strs[] = {content.c_str()};
  shader->setStrings(strs, 1);
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

vk::ShaderStageFlags GetStages(uint32_t mask) {
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

vk::ShaderStageFlagBits GetStage(EShLanguage flag) {
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
  
  return table.at(flag);
}

EShLanguage GetStage(vk::ShaderStageFlagBits flag) {
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

  return table.at(flag);
}

bool GetType(const glslang::TType& ttype, vk::DescriptorType& type) {
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

vk::PushConstantRange ToPushConstant(const glslang::TObjectReflection& obj) {
    auto pushCons = vk::PushConstantRange();
    pushCons.setSize(obj.size);
    pushCons.setStageFlags(GetStages(obj.stages));
    if (obj.offset > 0) {
        pushCons.setOffset(obj.offset);
    }
    return pushCons;
}

bool ToBinding(const glslang::TObjectReflection& obj,
    vk::DescriptorSetLayoutBinding& binding) {
    auto ttype = obj.getType();
    if (!GetType(*ttype, binding.descriptorType)) {
        return false;
    }
    binding.setBinding(ttype->getQualifier().layoutBinding);
    binding.setDescriptorCount(1);
    binding.setStageFlags(GetStages(obj.stages));
    return true;
}

class ShaderAnalyzer {
 public:
  ShaderAnalyzer(std::map<EShLanguage, const char*> shaders) {
    glslang::InitializeProcess();
    program_ = new glslang::TProgram();
    for (const auto& e : shaders) {
      auto* shader = new glslang::TShader(e.first);
      if (CompileShader(shader, e.second, e.first)) {
        program_->addShader(shader);
        shaders_[e.first] = shader;
      }
    }

    if (program_->link(kMessages)) {
      program_->buildReflection();
    }
  }
  ~ShaderAnalyzer() {
    for (auto& e : shaders_) {
        delete e.second;
        e.second = nullptr;
    }
    delete program_;
    glslang::FinalizeProcess();
  }

  void Query(ShaderData& data) {
      for (int32_t i = 0; i < program_->getNumUniformVariables(); i++) {
          auto& obj = program_->getUniform(i);
          auto  ttype = obj.getType();
      }

      for (int i = 0; i < program_->getNumUniformBlocks(); i++) {
          auto& obj = program_->getUniformBlock(i);
          auto  ttype = obj.getType();
      }

      for (int i = 0; i < program_->getNumBufferBlocks(); i++) {
          auto& obj = program_->getBufferBlock(i);
          auto  ttype = obj.getType();
      }

      for (int i = 0; i < program_->getNumPipeInputs(); i++) {
          auto obj = program_->getPipeInput(i);
          auto ttype = obj.getType();
      }
  }

 private:
  std::map<EShLanguage, glslang::TShader*> shaders_{};
  glslang::TProgram*                       program_{};
};

ShaderData LoadShader(std::map<vk::ShaderStageFlagBits, const char*> files) {

}

}  // namespace impl

}  // namespace VPP
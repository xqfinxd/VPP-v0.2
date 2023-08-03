#include "VPPShader.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <fstream>
#include <iostream>

#include "ShaderData.h"

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

namespace {

const int kGlslVersion = 400;
const glslang::EShSource kSourceLanguage = glslang::EShSourceGlsl;
const glslang::EShClient kClient = glslang::EShClientVulkan;
const glslang::EShTargetClientVersion kClientVersion =
    glslang::EShTargetVulkan_1_1;
const glslang::EShTargetLanguage kTargetLanguage = glslang::EShTargetSpv;
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
  std::ifstream file(fn); // 打开文件
  if (!file) {
    std::cerr << "Fail to open file: " << fn << std::endl;
    return false;
  }

  file.seekg(0, file.end);
  size_t length = file.tellg();
  ctx.resize(length + 1);
  file.seekg(0, file.beg);
  file.read(&ctx.at(0), length);

  file.close(); // 关闭文件
  return true;
}

glslang::TShader* CreateShader(const char* filename, EShLanguage stage) {
  auto shader_ptr = std::make_unique<glslang::TShader>(stage);
  if (filename == nullptr) {
    return nullptr;
  }

  TBuiltInResource resource;
  InitResources(resource);
  std::string content{};
  if (!LoadFile(filename, content)) {
    return nullptr;
  }

  const char* const strs[] = {content.c_str()};
  shader_ptr->setStrings(strs, 1);
  shader_ptr->setEntryPoint("main");
  shader_ptr->setEnvInput(kSourceLanguage, static_cast<EShLanguage>(stage),
                          kClient, kGlslVersion);
  shader_ptr->setEnvClient(kClient, kClientVersion);
  shader_ptr->setEnvTarget(kTargetLanguage, kTargetLanguageVersion);

  bool success = shader_ptr->parse(&resource, kGlslVersion, false, kMessages);
  if (success) {
    return shader_ptr.release();
  } else {
    std::cerr << shader_ptr->getInfoLog() << std::endl;
    return nullptr;
  }

  return nullptr;
}

glslang::TProgram* CreateProgram(std::vector<glslang::TShader*> shaders) {
  if (shaders.empty()) {
    std::cerr << "None shader is added to program" << std::endl;
    return nullptr;
  }

  auto program_ptr = std::make_unique<glslang::TProgram>();
  for (auto& shader : shaders) {
    program_ptr->addShader(shader);
  }

  bool success = program_ptr->link(kMessages);
  if (!success) {
    std::cerr << program_ptr->getInfoLog() << std::endl;
    return nullptr;
  }

  success = program_ptr->buildReflection();
  if (!success) {
    std::cerr << program_ptr->getInfoLog() << std::endl;
    return nullptr;
  } else {
    return program_ptr.release();
  }

  return nullptr;
}

bool FindStage(const char* fn, EShLanguage& stage) {
  auto extName = strrchr(fn, '.');
  if (!extName || strlen(extName) <= 1) {
    return false;
  }

  extName = extName + 1;
  if (strcmp(extName, "vert") == 0) {
    stage = EShLangVertex;
    return true;
  } else if (strcmp(extName, "frag") == 0) {
    stage = EShLangFragment;
    return true;
  } else if (strcmp(extName, "geom") == 0) {
    stage = EShLangGeometry;
    return true;
  } else if (strcmp(extName, "tesc") == 0) {
    stage = EShLangTessControl;
    return true;
  } else if (strcmp(extName, "tese") == 0) {
    stage = EShLangTessEvaluation;
    return true;
  } else if (strcmp(extName, "comp") == 0) {
    stage = EShLangCompute;
    return true;
  }

  return false;
}

vk::ShaderStageFlags GetStages(uint32_t mask) {
  const std::map<uint32_t, vk::ShaderStageFlagBits> table{
      {EShLangVertexMask, vk::ShaderStageFlagBits::eVertex},
      {EShLangTessControlMask, vk::ShaderStageFlagBits::eTessellationControl},
      {EShLangTessEvaluationMask,
       vk::ShaderStageFlagBits::eTessellationEvaluation},
      {EShLangGeometryMask, vk::ShaderStageFlagBits::eGeometry},
      {EShLangFragmentMask, vk::ShaderStageFlagBits::eFragment},
      {EShLangComputeMask, vk::ShaderStageFlagBits::eCompute},
  };

  vk::ShaderStageFlags flags{};

  for (const auto& e : table) {
    if (mask & e.first) {
      flags |= e.second;
    }
  }

  return flags;
}

vk::ShaderStageFlagBits GetStage(EShLanguage flag) {
  switch (flag) {
  case EShLangVertex:
    return vk::ShaderStageFlagBits::eVertex;
  case EShLangTessControl:
    return vk::ShaderStageFlagBits::eTessellationControl;
  case EShLangTessEvaluation:
    return vk::ShaderStageFlagBits::eTessellationEvaluation;
  case EShLangGeometry:
    return vk::ShaderStageFlagBits::eGeometry;
  case EShLangFragment:
    return vk::ShaderStageFlagBits::eFragment;
  default:
    break;
  }

  return (vk::ShaderStageFlagBits)~0;
}

} // namespace


class GlslInterpreter : public ShaderInterpreter {
  friend std::unique_ptr<ShaderInterpreter>
  ParseGlslShaders(const std::vector<const char*>& files);

public:
  GlslInterpreter(std::vector<const char*> files) : ShaderInterpreter(files) {
    glslang::InitializeProcess();

    for (auto fn : files) {
      if (!AddShader(fn)) return;
    }
    if (!Link()) return;

    success_ = true;
  }

  virtual ~GlslInterpreter() {
    if (program_) {
      delete program_;
    }

    for (auto& shader : shaders_) {
      if (shader) {
        delete shader;
      }
    }
    shaders_.clear();

    glslang::FinalizeProcess();
  }

  virtual uint32_t GetDescriptorSetCount() const {
    return 2;
  }

  virtual Bindings GetDescriptorSetBindings(uint32_t index) const {
    if (index >= descriptor_sets_.size()) return Bindings{};

    return descriptor_sets_[index];
  }

  virtual Stages GetAllStages() const {
    Stages stages; 
    for (auto shader : shaders_) {
      stages.push_back(GetStage(shader->getStage()));
    }
    return stages;
  }

  virtual Spirv GetStageSpirv(vk::ShaderStageFlagBits stage) const {
    Spirv spirv;
    for (const auto& shader : shaders_) {
      auto _stage = shader->getStage();
      if (GetStage(_stage) != stage) continue;

      if (auto temp = program_->getIntermediate(_stage)) {
        glslang::GlslangToSpv(*temp, spirv);
        break;
      }
    }
    return spirv;
  }

protected:
  virtual bool Success() const {
    return success_;
  }

  bool AddShader(const char* fn) {
    EShLanguage stage;

    bool found = FindStage(fn, stage);
    if (!found) {
      std::cerr << "Unsupported shader file : " << fn << "\n";
      return false;
    }

    for (const auto& shader : shaders_) {
      if (shader->getStage() == stage) {
        std::cerr << "Stages of conflict" << fn << "\n";
        return false;
      }
    }

    auto shader = CreateShader(fn, stage);
    if (!shader) return false;

    shaders_.push_back(shader);
    return true;
  }

  bool Link() {
    {
      auto program = CreateProgram(shaders_);
      if (!program) return false;
      program_ = program;
    }
    
    // check sampler
    for (int i = 0; i < program_->getNumUniformVariables(); i++) {
      auto& uniform = program_->getUniform(i);
      auto* ttype = uniform.getType();
      if (!ttype) continue;

      if (ttype->getBasicType() != glslang::EbtSampler) continue;

      if (!ttype->getSampler().isCombined()) return false;

      const auto& qualifier = ttype->getQualifier();
      auto set = qualifier.hasSet() ? qualifier.layoutSet : 0;
      auto binding = qualifier.hasBinding() ? qualifier.layoutBinding : 0;

      if (set != 1 || binding <= 0) return false;

      descriptor_sets_[1].emplace_back();
      descriptor_sets_[1].back().binding = binding;
      descriptor_sets_[1].back().descriptorCount = 1;
      descriptor_sets_[1].back().descriptorType = vk::DescriptorType::eCombinedImageSampler;
      descriptor_sets_[1].back().stageFlags = GetStages(uniform.stages);
    }

    // check unform block
    for (int i = 0; i < program_->getNumUniformBlocks(); i++) {
      auto& uniformBlock = program_->getUniformBlock(i);
      auto* ttype = uniformBlock.getType();
      if (!ttype) continue;

      const auto& qualifier = ttype->getQualifier();
      auto set = qualifier.hasSet() ? qualifier.layoutSet : 0;
      auto binding = qualifier.hasBinding() ? qualifier.layoutBinding : 0;

      if (qualifier.isPushConstant()) continue;

      if (qualifier.storage != glslang::EvqUniform) return false;
      if (set != 0 && (set != 1 || binding == 0)) return false;

      descriptor_sets_[0].emplace_back();
      descriptor_sets_[0].back().binding = binding;
      descriptor_sets_[0].back().descriptorCount = 1;
      descriptor_sets_[0].back().descriptorType =
          vk::DescriptorType::eUniformBuffer;
      descriptor_sets_[0].back().stageFlags = GetStages(uniformBlock.stages);
    }

    for (int i = 0; i < program_->getNumPipeInputs(); ++i) {
      auto& obj = program_->getPipeInput(i);
      auto* ttype = obj.getType();

      if (!ttype) continue;

      const auto& qualifier = ttype->getQualifier();
      auto location = qualifier.hasLocation() ? qualifier.layoutLocation : 0;
      
      if (location >= VertexType::VertexMaxCount) return false;

      locations_.push_back(location);
    }

    return true;
  }

private:
  std::vector<glslang::TShader*> shaders_{};
  glslang::TProgram* program_{};
  std::array<Bindings, 2> descriptor_sets_;
  std::vector<uint32_t> locations_;
  bool success_;
};

std::unique_ptr<ShaderInterpreter>
ParseGlslShaders(const std::vector<const char*>& files) {
  auto interpreter = std::make_unique<GlslInterpreter>(files);
  if (interpreter->Success()) return interpreter;
  return nullptr;
}
 
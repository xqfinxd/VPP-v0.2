#include "shader_loader.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <fstream>
#include <iostream>

#include "shader_data.h"

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

static void InitResources(TBuiltInResource& res) {
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

static bool LoadFile(const char* fn, std::string& ctx) {
  std::ifstream file(fn);  // 打开文件
  if (!file) {
    std::cerr << "Fail to open file: " << fn << std::endl;
    return false;
  }

  file.seekg(0, file.end);
  size_t length = file.tellg();
  ctx.resize(length + 1);
  file.seekg(0, file.beg);
  file.read(&ctx.at(0), length);

  file.close();  // 关闭文件
  return true;
}

static glslang::TShader* CreateShader(const char* filename, EShLanguage stage) {
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

static glslang::TProgram* CreateProgram(
    std::vector<glslang::TShader*> shaders) {
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

static bool FindStage(const char* fn, EShLanguage& stage) {
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

static vk::ShaderStageFlags GetStages(uint32_t mask) {
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

static vk::ShaderStageFlagBits GetStage(EShLanguage flag) {
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

vk::DescriptorType GetType(const glslang::TType& ttype) {
  const auto& qualifier = ttype.getQualifier();
  if (ttype.getBasicType() == glslang::EbtSampler) {
    const auto& sampler = ttype.getSampler();
    if (sampler.isCombined()) {
      return vk::DescriptorType::eCombinedImageSampler;
    } else if (sampler.isTexture()) {
      return vk::DescriptorType::eSampledImage;
    } else if (sampler.isTexture()) {
      return vk::DescriptorType::eSampledImage;
    } else if (sampler.isImage()) {
      return vk::DescriptorType::eStorageImage;
    } else if (sampler.isPureSampler()) {
      return vk::DescriptorType::eSampler;
    }
  } else if (ttype.getBasicType() == glslang::EbtBlock) {
    if (qualifier.storage == glslang::EvqUniform) {
      return vk::DescriptorType::eUniformBuffer;
    } else if (qualifier.storage == glslang::EvqBuffer) {
      return vk::DescriptorType::eStorageBuffer;
    }
  }

  return (vk::DescriptorType)~0;
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

vk::DescriptorSetLayoutBinding ToBinding(
    const glslang::TObjectReflection& obj) {
  vk::DescriptorSetLayoutBinding binding{};

  auto ttype = obj.getType();
  binding.setDescriptorType(GetType(*ttype));
  if (ttype->getQualifier().hasBinding()) {
    binding.setBinding(ttype->getQualifier().layoutBinding);
  } else {
    binding.setBinding(0);
  }
  binding.setDescriptorCount(1);
  binding.setStageFlags(GetStages(obj.stages));

  return binding;
}

class ShaderReader {
 public:
  ShaderReader() {
    glslang::InitializeProcess();
  }

  ~ShaderReader() {
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

  bool AddShader(const char* fn) {
    EShLanguage stage;

    bool found = FindStage(fn, stage);
    if (!found) {
      return false;
    }

    for (const auto& shader : shaders_) {
      if (shader->getStage() == stage) {
        return false;
      }
    }

    if (auto shader = CreateShader(fn, stage)) {
      shaders_.push_back(shader);
      return true;
    }

    return false;
  }

  bool Link() {
    if (auto program = CreateProgram(shaders_)) {
      program_ = program;
      return true;
    }

    return false;
  }

  void Query() {
    for (int32_t i = 0; i < program_->getNumUniformVariables(); i++) {
      auto& obj = program_->getUniform(i);
      auto* ttype = obj.getType();
      if (ttype->getBasicType() != glslang::EbtSampler) {
        continue;
      }

      uint32_t setNum = 0;
      if (ttype->getQualifier().hasSet()) {
        setNum = ttype->getQualifier().layoutSet;
      }

      data_.AddBinding(setNum, ToBinding(obj));
    }

    for (int i = 0; i < program_->getNumUniformBlocks(); i++) {
      auto& obj = program_->getUniformBlock(i);
      auto* ttype = obj.getType();
      if (ttype->getQualifier().isPushConstant()) {
        data_.AddPushConstant(ToPushConstant(obj));
        continue;
      }

      uint32_t setNum = 0;
      if (ttype->getQualifier().hasSet()) {
        setNum = ttype->getQualifier().layoutSet;
      }

      data_.AddBinding(setNum, ToBinding(obj));
    }

    for (int i = 0; i < program_->getNumBufferBlocks(); i++) {
      auto& obj = program_->getBufferBlock(i);
      auto* ttype = obj.getType();

      uint32_t setNum = 0;
      if (ttype->getQualifier().hasSet()) {
        setNum = ttype->getQualifier().layoutSet;
      }

      data_.AddBinding(setNum, ToBinding(obj));
    }

    for (int i = 0; i < program_->getNumPipeInputs(); i++) {
      auto& obj = program_->getPipeInput(i);
      auto* ttype = obj.getType();

      uint32_t location = 0;
      if (ttype->getQualifier().hasLocation()) {
        location = ttype->getQualifier().layoutLocation;
      }

      data_.AddLocation(location);
    }

    for (const auto& shader : shaders_) {
      auto stage = shader->getStage();
      if (auto temp = program_->getIntermediate(stage)) {
        std::vector<uint32_t> spv{};
        glslang::GlslangToSpv(*temp, spv);
        data_.AddSpvData(GetStage(stage), std::move(spv));
      }
    }
  }

  ShaderData& data() {
    return data_;
  }

 private:
  std::vector<glslang::TShader*> shaders_{};
  glslang::TProgram*             program_{};
  ShaderData                     data_{};
};

void ShaderData::AddBinding(uint32_t                         setNum,
                            vk::DescriptorSetLayoutBinding&& binding) {
  bool addSet = true;
  for (auto& ls : layout_sets_) {
    if (ls.set_num != setNum) {
      continue;
    }

    auto target = binding.binding;
    auto it = std::find_if(ls.bindings.begin(), ls.bindings.end(),
                           [target](vk::DescriptorSetLayoutBinding& lb) {
                             return lb.binding == target;
                           });

    if (it != ls.bindings.end()) {
      it->descriptorCount += binding.descriptorCount;
    } else {
      ls.bindings.insert(it, binding);
    }

    break;
  }

  if (addSet) {
    layout_sets_.emplace_back();
    layout_sets_.back().set_num = setNum;
    layout_sets_.back().bindings.push_back(binding);
  }
}

void ShaderData::AddPushConstant(vk::PushConstantRange&& pushConst) {
  push_constants_.push_back(pushConst);
}

void ShaderData::AddLocation(uint32_t location) {
  if (!std::count(locations_.begin(), locations_.end(), location)) {
    locations_.push_back(location);
  }
}

void ShaderData::AddSpvData(vk::ShaderStageFlagBits stage,
                            std::vector<uint32_t>&& data) {
  spv_datas_.emplace_back();
  spv_datas_.back().stage = stage;
  spv_datas_.back().data.swap(data);
}

ShaderReader* LoadShader(std::vector<const char*> files) {
  auto loader = std::make_unique<ShaderReader>();

  for (auto fn : files) {
    if (!loader->AddShader(fn)) {
      return nullptr;
    }
  }

  if (!loader->Link()) {
    return nullptr;
  }

  loader->Query();

  return loader.release();
}

void DestroyShader(ShaderReader* loader) {
  if (loader) {
    delete loader;
  }
}

ShaderData* GetShaderData(ShaderReader* loader) {
  if (loader) {
    return &loader->data();
  }
  return nullptr;
}

}  // namespace impl

}  // namespace VPP
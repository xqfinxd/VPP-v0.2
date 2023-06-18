#include "ShaderReader.h"

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

namespace Shader {

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

static glslang::TProgram*
CreateProgram(std::vector<glslang::TShader*> shaders) {
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

static inline bool GetVkType(const glslang::TType* ttype,
                             vk::DescriptorType& type, bool* isSampler) {
  if (!ttype) {
    return false;
  }
  const auto& qualifier = ttype->getQualifier();
  if (ttype->getBasicType() == glslang::EbtSampler) {
    const auto& sampler = ttype->getSampler();
    if (sampler.isCombined()) {
      type = vk::DescriptorType::eCombinedImageSampler;
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
    }
  } else if (ttype->getBasicType() == glslang::EbtBlock) {
    if (qualifier.storage == glslang::EvqUniform) {
      type = vk::DescriptorType::eUniformBuffer;
      return true;
    } else if (qualifier.storage == glslang::EvqBuffer) {
      type = vk::DescriptorType::eStorageBuffer;
      return true;
    }
  }

  return false;
}

static void AddUniform(std::vector<Shader::Uniform>& uniforms,
                       const glslang::TObjectReflection& obj,
                       const glslang::TType* ttype) {
  vk::DescriptorType descType = (vk::DescriptorType)~0;
  bool isSampler = false;
  if (!GetVkType(ttype, descType, &isSampler)) {
    return;
  }

  Shader::Uniform uniform{};
  {
    const auto& q = ttype->getQualifier();
    uniform.set = q.hasSet() ? q.layoutSet : 0;
    uniform.binding = q.hasBinding() ? q.layoutBinding : 0;
    uniform.stages = GetStages(obj.stages);
    uniform.type = descType;
    uniform.count = isSampler ? obj.size : 1;
  }

  auto iter = std::find_if(uniforms.begin(), uniforms.end(),
                           [&uniform](const Shader::Uniform& e) {
                             return e.IsSame(uniform);
                           });
  if (iter == uniforms.end()) {
    uniforms.push_back(uniform);
  } else {
    iter->count += uniform.count;
  }
}

struct ReaderImpl {
  ReaderImpl() {
    glslang::InitializeProcess();
  }

  ~ReaderImpl() {
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

  void Query(Shader::MetaData& data_) const {
    Shader::MetaData data{};

    for (int32_t i = 0; i < program_->getNumUniformVariables(); i++) {
      auto& obj = program_->getUniform(i);
      auto* ttype = obj.getType();

      AddUniform(data.uniforms, obj, ttype);
    }

    for (int i = 0; i < program_->getNumUniformBlocks(); i++) {
      auto& obj = program_->getUniformBlock(i);
      auto* ttype = obj.getType();

      if (ttype && ttype->getQualifier().isPushConstant()) {
        Shader::PushConstant pushCons{};
        pushCons.size = obj.size;
        pushCons.stages = GetStages(obj.stages);

        data.pushes.push_back(pushCons);
        continue;
      }

      AddUniform(data.uniforms, obj, ttype);
    }

    for (int i = 0; i < program_->getNumBufferBlocks(); i++) {
      auto& obj = program_->getBufferBlock(i);
      auto* ttype = obj.getType();

      AddUniform(data.uniforms, obj, ttype);
    }

    std::sort(data.uniforms.begin(), data.uniforms.end());
    if (!data.uniforms.empty() && data.uniforms[0].set != 0) {
      return;
    }
    for (size_t i = 0; i + 1 < data.uniforms.size(); ++i) {
      if (data.uniforms[i + 1].set - data.uniforms[i].set > 1) {
        return;
      }
    }

    for (const auto& shader : shaders_) {
      auto stage = shader->getStage();
      if (auto temp = program_->getIntermediate(stage)) {
        data.spvs.emplace_back();
        auto& spv = data.spvs.back();
        spv.stage = GetStage(stage);
        spv.data.clear();
        glslang::GlslangToSpv(*temp, spv.data);
      }
    }

    for (int i = 0; i < program_->getNumPipeInputs(); ++i) {
      auto& obj = program_->getPipeInput(i);
      auto* ttype = obj.getType();
      if (!ttype) {
        continue;
      }
      Shader::Input input{};
      {
        const auto& q = ttype->getQualifier();
        if (q.hasLocation()) {
          input.location = q.layoutLocation;
        }
      }
      data.inputs.push_back(input);
    }
    std::sort(data.inputs.begin(), data.inputs.end());
    if (!data.inputs.empty() && data.inputs[0].location != 0) {
      return;
    }
    for (size_t i = 0; i + 1 < data.inputs.size(); ++i) {
      if (data.inputs[i + 1].location - data.inputs[i].location > 1) {
        return;
      }
    }

    data_.Swap(std::move(data));
  }

private:
  std::vector<glslang::TShader*> shaders_{};
  glslang::TProgram* program_{};
};

Reader::Reader(std::vector<const char*> files) {
  auto impl = std::make_unique<ReaderImpl>();
  do {
    for (auto fn : files) {
      if (!impl->AddShader(fn)) {
        break;
      }
    }

    if (!impl->Link()) {
      break;
    }

    impl_ = impl.release();
  } while (false);
}

Reader::Reader() : impl_(nullptr) {
}

Reader::~Reader() {
  if (impl_) {
    delete impl_;
  }
}

bool Reader::GetData(MetaData* data) {
  if (!impl_ || !data) {
    return false;
  }

  impl_->Query(*data);
  return true;
}

} // namespace Shader
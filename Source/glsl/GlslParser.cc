#include "GlslParser.h"

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

namespace {

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

vk::ShaderStageFlags GetVulkanStages(uint32_t mask) {
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

vk::DescriptorType GetSamplerType(const glslang::TSampler& sampler) {
  vk::DescriptorType type = vk::DescriptorType::eCombinedImageSampler;

  if (sampler.isTexture()) { // hlsl ?
    type = vk::DescriptorType::eSampledImage;
  } else if (sampler.isPureSampler()) { // hlsl ?
    type = vk::DescriptorType::eSampler;
  } else if (sampler.isSubpass()) {
    type = vk::DescriptorType::eInputAttachment;
  } else if (sampler.isImage()) { // rw
    type = vk::DescriptorType::eStorageImage;
  }
  return type;
}

vk::DescriptorType GetBlockType(const glslang::TType& ttype) {
  vk::DescriptorType type = vk::DescriptorType::eUniformBuffer;

  auto storage = ttype.getQualifier().storage;
  if (storage == glslang::TStorageQualifier::EvqBuffer) { // rw
    type = vk::DescriptorType::eStorageBuffer;
  }
  return type;
}

} // namespace

class GlslParserImpl {
  friend GlslParser;
  using ShaderMap =
      std::map<vk::ShaderStageFlagBits, std::unique_ptr<glslang::TShader>>;
  using Program = std::unique_ptr<glslang::TProgram>;
  using VulkanVersion = glslang::EShTargetClientVersion;
  using SpirvVersion = glslang::EShTargetLanguageVersion;

public:
  void Clear() {
    m_Program.reset();
    m_Shaders.clear();
  }

  bool Loaded() {
    return !m_Shaders.empty() && m_Program;
  }

  bool LoadShader(const GlslSource& src) {
    if (src.m_Content.empty()) return false;

    EShLanguage tempStage;

    switch (src.m_Stage) {
    case vk::ShaderStageFlagBits::eVertex:
      tempStage = EShLanguage::EShLangVertex;
      break;
    case vk::ShaderStageFlagBits::eTessellationControl:
      tempStage = EShLanguage::EShLangTessControl;
      break;
    case vk::ShaderStageFlagBits::eTessellationEvaluation:
      tempStage = EShLanguage::EShLangTessEvaluation;
      break;
    case vk::ShaderStageFlagBits::eGeometry:
      tempStage = EShLanguage::EShLangGeometry;
      break;
    case vk::ShaderStageFlagBits::eFragment:
      tempStage = EShLanguage::EShLangFragment;
      break;
    default:
      return false;
      break;
    }

    auto tempShader = std::make_unique<glslang::TShader>(tempStage);

    TBuiltInResource defaultResource;
    InitResources(defaultResource);

    const char* const strs[] = {src.m_Content.c_str()};

    tempShader->setStrings(strs, 1);
    if (!src.m_Entry.empty())
      tempShader->setEntryPoint(src.m_Entry.c_str());
    else
      tempShader->setEntryPoint("main");
    tempShader->setEnvInput(glslang::EShSourceGlsl, tempStage,
                            glslang::EShClientVulkan, m_GlslVersion);
    tempShader->setEnvClient(glslang::EShClientVulkan, m_VulkanVersion);
    tempShader->setEnvTarget(glslang::EShTargetSpv, m_SpirvVersion);

    uint32_t messageFlags = EShMsgSpvRules | EShMsgVulkanRules | EShMsgAST;

    bool success = tempShader->parse(&defaultResource, m_GlslVersion, false,
                                     (EShMessages)messageFlags);
    if (!success) {
      std::cerr << tempShader->getInfoLog() << std::endl;
      return false;
    }

    m_Shaders[src.m_Stage].swap(tempShader);
    return true;
  }

  bool LinkShaders() {
    if (m_Shaders.empty()) return false;

    auto tempProgram = std::make_unique<glslang::TProgram>();
    for (auto& shader : m_Shaders) {
      tempProgram->addShader(shader.second.get());
    }

    uint32_t messageFlags = EShMsgSpvRules | EShMsgVulkanRules | EShMsgAST;

    bool success = tempProgram->link((EShMessages)messageFlags);
    if (!success) {
      std::cerr << tempProgram->getInfoLog() << std::endl;
      return false;
    }

    success = tempProgram->buildReflection();
    if (!success) {
      std::cerr << tempProgram->getInfoLog() << std::endl;
      return false;
    }

    m_Program.swap(tempProgram);
    return true;
  }

private:
  ShaderMap m_Shaders{};
  Program m_Program{};

  int m_GlslVersion = 400;
  VulkanVersion m_VulkanVersion = VulkanVersion::EShTargetVulkan_1_1;
  SpirvVersion m_SpirvVersion = SpirvVersion::EShTargetSpv_1_2;
};

GlslParser::GlslParser() {
  glslang::InitializeProcess();
  m_Impl = new GlslParserImpl;
}

GlslParser::~GlslParser() {
  delete m_Impl;
  glslang::FinalizeProcess();
}

bool GlslParser::Load(const std::vector<GlslSource>& contents) {
  m_Impl->Clear();

  for (const auto& src : contents) {
    if (!m_Impl->LoadShader(src)) return false;
  }

  if (!m_Impl->LinkShaders()) return false;

  return m_Impl->Loaded();
}

std::vector<GlslUniform> GlslParser::GetUniforms() const {
  std::vector<GlslUniform> result;
  if (!m_Impl->Loaded()) return result;

  const auto program = m_Impl->m_Program.get();
  for (int i = 0; i < program->getNumUniformVariables(); i++) {
    auto& uniform = program->getUniform(i);
    auto* ttype = uniform.getType();

    if (ttype->getBasicType() != glslang::EbtSampler) continue;

    result.emplace_back();
    auto& sampler = result.back();

    if (ttype->getQualifier().hasBinding())
      sampler.m_Binding = ttype->getQualifier().layoutBinding;
    else
      sampler.m_Binding = 0;
    if (ttype->getQualifier().hasSet())
      sampler.m_Set = ttype->getQualifier().layoutSet;
    else
      sampler.m_Set = 0;
    sampler.m_Name = uniform.name;
    sampler.m_Stages = GetVulkanStages(uniform.stages);
    sampler.m_Type = GetSamplerType(ttype->getSampler());
  }

  for (int i = 0; i < program->getNumUniformBlocks(); i++) {
    auto& uniformBlock = program->getUniformBlock(i);
    auto* ttype = uniformBlock.getType();

    if (ttype->getQualifier().isPushConstant()) continue;

    result.emplace_back();
    auto& block = result.back();

    if (ttype->getQualifier().hasBinding())
      block.m_Binding = ttype->getQualifier().layoutBinding;
    else
      block.m_Binding = 0;
    if (ttype->getQualifier().hasSet())
      block.m_Set = ttype->getQualifier().layoutSet;
    else
      block.m_Set = 0;
    block.m_Name = uniformBlock.name;
    block.m_Stages = GetVulkanStages(uniformBlock.stages);
    block.m_Type = GetBlockType(*ttype);
    block.m_Size = uniformBlock.size;

    for (const auto& st : *ttype->getStruct()) {
      block.m_Members.emplace_back();
      auto& mem = block.m_Members.back();

      mem.m_Name = st.type->getFieldName().c_str();
      mem.m_Offset = st.type->getQualifier().layoutOffset;
    }
  }

  return result;
}

std::vector<GlslVertexAttribute> GlslParser::GetVertexAttributes() const {
  std::vector<GlslVertexAttribute> result;
  if (!m_Impl->Loaded()) return result;

  const auto program = m_Impl->m_Program.get();
  for (int i = 0; i < program->getNumPipeInputs(); ++i) {
    auto& pipeInput = program->getPipeInput(i);
    auto* ttype = pipeInput.getType();

    result.emplace_back();
    auto& attrib = result.back();

    if (ttype->getQualifier().hasLocation())
      attrib.m_Location = ttype->getQualifier().layoutLocation;
    else
      attrib.m_Location = 0;
    attrib.m_Name = pipeInput.name;
    attrib.m_Size = pipeInput.size;
    attrib.m_Stages = GetVulkanStages(pipeInput.stages);
  }

  return result;
}

std::vector<GlslPushConstant> GlslParser::GetPushConstants() const {
  std::vector<GlslPushConstant> result;
  if (!m_Impl->Loaded()) return result;

  const auto program = m_Impl->m_Program.get();
  for (int i = 0; i < program->getNumUniformBlocks(); i++) {
    auto& uniformBlock = program->getUniformBlock(i);
    auto* ttype = uniformBlock.getType();

    if (!ttype->getQualifier().isPushConstant()) continue;

    result.emplace_back();
    auto& pushConstant = result.back();

    pushConstant.m_Stages = GetVulkanStages(uniformBlock.stages);
    pushConstant.m_Name = uniformBlock.name;
    pushConstant.m_Size = uniformBlock.size;

    for (const auto& st : *ttype->getStruct()) {
      pushConstant.m_Members.emplace_back();
      auto& mem = pushConstant.m_Members.back();

      mem.m_Name = st.type->getFieldName().c_str();
      mem.m_Offset = st.type->getQualifier().layoutOffset;
    }
  }

  return result;
}

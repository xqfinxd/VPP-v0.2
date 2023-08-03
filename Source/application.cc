#include "Application.h"

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "impl/Buffer.h"
#include "impl/Device.h"
#include "impl/DrawCmd.h"
#include "impl/Image.h"
#include "impl/Pipeline.h"
#include "impl/ShaderData.h"
#include "impl/VPPImage.h"
#include "impl/VPPShader.h"
#include "impl/Window.h"

namespace VPP {

static impl::Window* g_Window = nullptr;
static impl::Device* g_Device = nullptr;
static impl::FrameData* frameData = nullptr;
static impl::PipelineInfo* basicPipe = nullptr;
static impl::VertexBuffer* vertexBuffer = nullptr;
static impl::VertexArray* vertexArray = nullptr;
static impl::IndexBuffer* indexBuffer = nullptr;
static impl::ShaderPass* renderPath = nullptr;
static impl::DrawParam* cmd = nullptr;
static impl::SamplerTexture* tex1 = nullptr;
static impl::SamplerTexture* tex2 = nullptr;
static impl::UniformBuffer* transform = nullptr;

Application::Application() {}

Application::~Application() {}

void Application::Run() {
  impl::WindowOption options;
  options.width = 1280;
  options.height = 900;
  options.fps = 30;
  strcpy_s<64>(options.title, "VPP");
  g_Window = new impl::Window(options);
  g_Device = new impl::Device(g_Window);
  frameData = new impl::FrameData();

  OnStart();

  while (!g_Window->ShouldClose()) {
    g_Window->StartFrame(*frameData);

    if (!g_Window->IsMinimized()) {
      OnLoop();
    }

    g_Window->EndFrame(*frameData);
  }
  g_Device->EndDraw();
  OnEnd();

  delete frameData;
  frameData = nullptr;
  delete g_Device;
  g_Device = nullptr;
  delete g_Window;
  g_Window = nullptr;
}

void Application::OnStart() {

  std::vector<float> vertices = {
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
      0.5f,  -0.5f, -0.5f, 1.0f, 0.0f,
      0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f,
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,
      0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
      -0.5f, 0.5f,  0.5f,  0.0f, 1.0f,
      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,

      -0.5f, 0.5f,  0.5f,  1.0f, 0.0f,
      -0.5f, 0.5f,  -0.5f, 1.0f, 1.0f,
      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,
      -0.5f, 0.5f,  0.5f,  1.0f, 0.0f,

      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      0.5f,  -0.5f, -0.5f, 0.0f, 1.0f,
      0.5f,  -0.5f, -0.5f, 0.0f, 1.0f,
      0.5f,  -0.5f, 0.5f,  0.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
      0.5f,  -0.5f, -0.5f, 1.0f, 1.0f,
      0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
      0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,
      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

      -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f,
      0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      -0.5f, 0.5f,  0.5f,  0.0f, 0.0f,
      -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f
  };

  std::vector<uint32_t> indices = {
      0, 1, 3, // first triangle
      1, 2, 3  // second triangle
  };

  vertexBuffer = new impl::VertexBuffer(g_Device);
  vertexBuffer->SetData((uint32_t)sizeof(float) * 5, 36, vertices.data(),
                        vertices.size() * sizeof(float));

  indexBuffer = new impl::IndexBuffer(g_Device);
  indexBuffer->SetData((uint32_t)indices.size(), indices.data(),
                       indices.size() * sizeof(uint32_t));

  vertexArray = new impl::VertexArray(g_Device);
  vertexArray->BindBuffer(*vertexBuffer);
  vertexArray->SetVertexAttrib(0, 0, vk::Format::eR32G32B32Sfloat, 0);
  vertexArray->SetVertexAttrib(1, 0, vk::Format::eR32G32Sfloat,
                             (3 * sizeof(float)));
  //vertexArray->BindBuffer(*indexBuffer);
  /*vertexArray->SetVertexAttrib(2, 0, vk::Format::eR32G32Sfloat,
                               (6 * sizeof(float)));*/

  tex1 = new impl::SamplerTexture(g_Device);
  tex2 = new impl::SamplerTexture(g_Device);
  stb::Reader reader;
  reader.Load("awesomeface.png", 4);
  tex1->SetImage2D(vk::Format::eR8G8B8A8Unorm, reader.width(), reader.height(),
                   4, reader.pixel());
  reader.Load("container.jpg", 4);
  tex2->SetImage2D(vk::Format::eR8G8B8A8Unorm, reader.width(), reader.height(),
                   4, reader.pixel());

  transform = new impl::UniformBuffer(g_Device);
  transform->SetData(sizeof(glm::mat4) * 3);

  basicPipe = new impl::PipelineInfo(g_Device);
  {
    if (auto data = ParseGlslShaders({"basic.vert", "basic.frag"}))
      basicPipe->SetShader(data.get());
  }

  renderPath = new impl::ShaderPass(g_Device);

  g_Device->InitRenderPath(renderPath);
  cmd = new impl::DrawParam(g_Device);
  cmd->Bind(basicPipe, vertexArray, renderPath);

  cmd->SetTexture(1, tex1);
  cmd->SetTexture(2, tex2);
  cmd->SetUniform(0, transform);

  std::vector<vk::ClearValue> clearValues = {
      vk::ClearValue().setColor(vk::ClearColorValue{0.2f, 0.3f, 0.3f, 1.0f}),
      vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue{1.0f, 0}),
  };
  cmd->SetClearValues(clearValues);
}

void Application::OnLoop() {
  static glm::vec3 position{0.f};
  static glm::vec3 cameraPos{10.f, 0.f, 10.f};
  
  for (const auto& e : frameData->dump_events)
  {
    if (e.type == SDL_KEYUP) {
      switch (e.key.keysym.sym) {
      case SDLK_LEFT:
        position.x--;
        break;
      case SDLK_RIGHT:
        position.x++;
        break;
      case SDLK_UP:
        position.y++;
        break;
      case SDLK_DOWN:
        position.y--;
        break;
      default:
        break;
      }
    }
    if (e.type == SDL_MOUSEWHEEL) {
      position.z += e.wheel.y;
    }
  }
  glm::mat4 view = glm::mat4(
      1.0f); // make sure to initialize matrix to identity matrix first
  float radius = 10.0f;
  cameraPos.x = static_cast<float>(sin(/*SDL_GetTicks() / 100*/ 0.f) * radius);
  cameraPos.y = static_cast<float>(cos(/*SDL_GetTicks() / 100*/ 0.f) * radius);
  view = glm::lookAt(cameraPos, position,
                     glm::vec3(0.0f, 1.0f, 0.0f));
  // calculate the model matrix for each object and pass it to shader before
  // drawing
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, position);
  float angle = 20.0f * 0;
  model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
  int winWidth = 0, winHeight = 0;
  g_Window->GetSize(winWidth, winHeight);
  glm::mat4 projection = glm::perspective(
      glm::radians(45.0f), (float)winWidth / (float)winHeight, 0.1f, 100.0f);

  glm::mat4 bytes[3] = {model, view, projection};
  transform->UpdateData(bytes, sizeof(glm::mat4) * 3);
  g_Device->PrepareRender();
  g_Device->Render(cmd);
  g_Device->Render(cmd);
  g_Device->FinishRender();
}

void Application::OnEnd() {
  delete transform;
  delete tex2;
  delete tex1;
  delete basicPipe;
  delete vertexArray;
  delete indexBuffer;
  delete vertexBuffer;
  delete cmd;
  delete renderPath;
}
} // namespace VPP

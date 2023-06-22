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
#include "VPP_Config.h"

namespace VPP {

namespace impl {

Window* g_Window = nullptr;
Device* g_Device = nullptr;

} // namespace impl

static impl::Pipeline* basicPipe = nullptr;
static impl::VertexBuffer* vertexBuffer = nullptr;
static impl::VertexArray* vertexArray = nullptr;
static impl::IndexBuffer* indexBuffer = nullptr;
static impl::DrawParam* cmd = nullptr;
static impl::SamplerTexture* tex1 = nullptr;
static impl::SamplerTexture* tex2 = nullptr;
static impl::UniformBuffer* transform = nullptr;

Application::Application() {}

Application::~Application() {}

void Application::Run() {
  impl::g_Window = new impl::Window;
  impl::g_Device = new impl::Device(impl::g_Window);

  OnStart();

  impl::WindowFrameData frameData{};
  while (impl::g_Window->running()) {
    impl::g_Window->StartFrame(frameData);

    if (!impl::g_Window->IsMinimized()) {
      OnLoop();
    }

    impl::g_Window->EndFrame(frameData);
  }
  impl::g_Device->EndDraw();
  OnEnd();

  delete impl::g_Device;
  impl::g_Device = nullptr;
  delete impl::g_Window;
  impl::g_Window = nullptr;
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

  vertexBuffer = new impl::VertexBuffer();
  vertexBuffer->SetData((uint32_t)sizeof(float) * 5, 36, vertices.data(),
                        vertices.size() * sizeof(float));

  indexBuffer = new impl::IndexBuffer();
  indexBuffer->SetData((uint32_t)indices.size(), indices.data(),
                       indices.size() * sizeof(uint32_t));

  vertexArray = new impl::VertexArray();
  vertexArray->BindBuffer(*vertexBuffer);
  //vertexArray->BindBuffer(*indexBuffer);

  tex1 = new impl::SamplerTexture();
  tex2 = new impl::SamplerTexture();
  Image::Reader reader;
  reader.Load("awesomeface.png", 4);
  tex1->SetImage2D(vk::Format::eR8G8B8A8Srgb, reader.width(), reader.height(),
                   4, reader.pixel());
  reader.Load("container.jpg", 4);
  tex2->SetImage2D(vk::Format::eR8G8B8A8Srgb, reader.width(), reader.height(),
                   4, reader.pixel());

  transform = new impl::UniformBuffer();
  transform->SetData(sizeof(glm::mat4) * 3);

  basicPipe = new impl::Pipeline();
  {
    Shader::Reader reader({"basic.vert", "basic.frag"});
    Shader::MetaData data{};
    if (reader.GetData(&data))
      basicPipe->SetShader(data);
    basicPipe->SetVertexAttrib(0, 0, vk::Format::eR32G32B32Sfloat, 0);
    basicPipe->SetVertexAttrib(1, 0, vk::Format::eR32G32Sfloat,
                               (3 * sizeof(float)));
    /*basicPipe->SetVertexAttrib(2, 0, vk::Format::eR32G32Sfloat,
                               (6 * sizeof(float)));*/
  }

  cmd = new impl::DrawParam();
  cmd->set_vertices(*vertexArray);

  cmd->set_pipeline(*basicPipe);

  cmd->add_sampler_texture(0, *tex1);
  cmd->add_sampler_texture(1, *tex2);
  cmd->BindTexture(0, 0, 0);
  cmd->BindTexture(1, 0, 1);

  cmd->add_uniform_buffer(0, *transform);
  cmd->BindBlock(0, 1, 0);

  std::vector<vk::ClearValue> clearValues = {
      vk::ClearValue().setColor(vk::ClearColorValue{0.2f, 0.3f, 0.3f, 1.0f}),
      vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue{1.0f, 0}),
  };
  cmd->set_clear_values(clearValues);
  impl::g_Device->set_cmd(*cmd);
}

void Application::OnLoop() {
  glm::mat4 model = glm::mat4(
      1.0f); // make sure to initialize matrix to identity matrix first
  glm::mat4 view = glm::mat4(1.0f);
  glm::mat4 projection = glm::mat4(1.0f);
  model = glm::rotate(model, (float)SDL_GetTicks() / 1000, glm::vec3(0.5f, 1.0f, 0.0f));
  view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
  projection = glm::perspective(
      glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
  glm::mat4 bytes[3] = {model, view, projection};
  transform->UpdateData(bytes, sizeof(glm::mat4) * 3);

  impl::g_Device->Draw();
}

void Application::OnEnd() {
  delete transform;
  delete tex2;
  delete tex1;
  delete basicPipe;
  delete vertexArray;
  delete indexBuffer;
  delete vertexBuffer;
}
} // namespace VPP

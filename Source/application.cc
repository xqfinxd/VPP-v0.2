#include "Application.h"

#include <iostream>

#include "impl/Buffer.h"
#include "impl/Device.h"
#include "impl/DrawCmd.h"
#include "impl/Pipeline.h"
#include "impl/ShaderData.h"
#include "impl/VPPShader.h"
#include "impl/VPPImage.h"
#include "impl/Window.h"

namespace VPP {

namespace impl {

Window* g_Window = nullptr;
Device* g_Device = nullptr;

} // namespace impl

static impl::Pipeline* basicPipe = nullptr;
static impl::VertexBuffer* vertexBuffer = nullptr;
static impl::VertexArray* vertexArray = nullptr;
static impl::IndexBuffer* indexBuffer = nullptr;
static impl::DrawCmd* cmd = nullptr;

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
      // positions         // colors
      0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom right
      -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom left
      0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f  // top
  };

  std::vector<uint32_t> indices = {
      // note that we start from 0!
      0, 1, 2, // first Triangle
      1, 2, 0  // second Triangle
  };

  vertexBuffer = new impl::VertexBuffer();
  vertexBuffer->SetData((uint32_t)sizeof(float) * 6, 3, vertices.data(),
                        vertices.size() * sizeof(float));

  indexBuffer = new impl::IndexBuffer();
  indexBuffer->SetData((uint32_t)indices.size(), indices.data(),
                       indices.size() * sizeof(uint32_t));

  vertexArray = new impl::VertexArray();
  vertexArray->BindBuffer(*vertexBuffer);
  vertexArray->BindBuffer(*indexBuffer);

  {
      Image::Reader reader;
      reader.Load("container.jpg", 0);
      reader.pixel();
  }

  basicPipe = new impl::Pipeline();
  {
    Shader::Reader reader({"basic.vert", "basic.frag"});
    Shader::MetaData data{};
    if (reader.GetData(&data))
      basicPipe->SetShader(data);
    basicPipe->SetVertexArray(*vertexArray);
    basicPipe->SetVertexAttrib(0, 0, vk::Format::eR32G32B32Sfloat, 0);
    basicPipe->SetVertexAttrib(1, 0, vk::Format::eR32G32B32Sfloat,
                               (3 * sizeof(float)));
    basicPipe->Enable();
  }

  cmd = new impl::DrawCmd();
  cmd->set_vertices(*vertexArray);
  cmd->set_pipeline(*basicPipe);
  std::vector<vk::ClearValue> clearValues = {
      vk::ClearValue().setColor(vk::ClearColorValue{0.2f, 0.3f, 0.3f, 1.0f}),
      vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue{1.0f, 0}),
  };
  cmd->set_clear_values(clearValues);
  impl::g_Device->set_cmd(*cmd);
}

void Application::OnLoop() { impl::g_Device->Draw(); }

void Application::OnEnd() {
  delete basicPipe;
  delete vertexArray;
  delete indexBuffer;
  delete vertexBuffer;
}
} // namespace VPP
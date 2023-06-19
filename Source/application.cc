#include "Application.h"

#include <iostream>

#include "impl/Device.h"
#include "impl/Buffer.h"
#include "impl/Pipeline.h"
#include "impl/ShaderData.h"
#include "impl/ShaderReader.h"
#include "impl/Window.h"
#include "impl/DrawCmd.h"

namespace VPP {

namespace impl {

Window* g_Window = nullptr;
Device* g_Device = nullptr;

} // namespace impl

struct {
  impl::Pipeline* basicPipe = nullptr;
  impl::VertexBuffer* vertexBuffer = nullptr;
  impl::VertexArray* vertexArray = nullptr;
  impl::IndexBuffer* indexBuffer = nullptr;
  impl::DrawCmd* cmd = nullptr;
} _G{};

Application::Application() {
}

Application::~Application() {
}

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
      0.5f,  0.5f,  0.0f, // top right
      0.5f,  -0.5f, 0.0f, // bottom right
      -0.5f, -0.5f, 0.0f, // bottom left
      -0.5f, 0.5f,  0.0f  // top left
  };

  std::vector<uint32_t> indices = {
      // note that we start from 0!
      0, 1, 3, // first Triangle
      1, 2, 3  // second Triangle
  };

  _G.vertexBuffer = new impl::VertexBuffer();
  _G.vertexBuffer->SetData((uint32_t)sizeof(float) * 3, 4, vertices.data(),
                           vertices.size() * sizeof(float));

  _G.indexBuffer = new impl::IndexBuffer();
  _G.indexBuffer->SetData((uint32_t)indices.size(), indices.data(),
                          indices.size() * sizeof(uint32_t));

  _G.vertexArray = new impl::VertexArray();
  _G.vertexArray->add_vertex(*_G.vertexBuffer);
  _G.vertexArray->set_index(*_G.indexBuffer);

  _G.basicPipe = new impl::Pipeline();
  {
    Shader::Reader reader({"basic.vert", "basic.frag"});
    Shader::MetaData data{};
    if (reader.GetData(&data))
      _G.basicPipe->SetShader(data);
    _G.basicPipe->SetVertexArray(*_G.vertexArray);
    _G.basicPipe->SetVertexAttrib(0, 0, vk::Format::eR32G32B32Sfloat, 0);
    _G.basicPipe->Enable();
  }

  _G.cmd = new impl::DrawCmd();
  _G.cmd->set_vertices(*_G.vertexArray);
  _G.cmd->set_pipeline(*_G.basicPipe);
  std::vector<vk::ClearValue> clearValues = {
      vk::ClearValue().setColor(vk::ClearColorValue{0.2f, 0.3f, 0.3f, 1.0f}),
      vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue{1.0f, 0}),
  };
  _G.cmd->set_clear_values(clearValues);
  impl::g_Device->set_cmd(*_G.cmd);
  
}

void Application::OnLoop() {
  impl::g_Device->Draw();
}

void Application::OnEnd() {
  delete _G.basicPipe;
  delete _G.vertexArray;
  delete _G.indexBuffer;
  delete _G.vertexBuffer;
}
} // namespace VPP
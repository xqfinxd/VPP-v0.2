#include "Application.h"

#include <iostream>

#include "impl/Device.h"
#include "impl/Pipeline.h"
#include "impl/ShaderData.h"
#include "impl/ShaderReader.h"
#include "impl/Window.h"

namespace VPP {

struct {
  std::shared_ptr<impl::Window> window = nullptr;
  std::shared_ptr<impl::Device> device = nullptr;
  impl::Pipeline* basicPipe = nullptr;
  impl::VertexBuffer* vertexBuffer = nullptr;
  impl::VertexArray* vertexArray = nullptr;
  impl::IndexBuffer* indexBuffer = nullptr;

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

} _G{};

Application::Application() {
}

Application::~Application() {
}

void Application::Run() {

  OnStart();

  impl::WindowFrameData frameData{};
  while (_G.window->running()) {
    _G.window->StartFrame(frameData);

    OnLoop();

    _G.window->EndFrame(frameData);
  }
  _G.device->EndDraw();
  OnEnd();
}

void Application::OnStart() {
  _G.window = std::make_shared<impl::Window>();
  _G.device = std::make_shared<impl::Device>(_G.window);

  _G.vertexBuffer = new impl::VertexBuffer();
  _G.vertexBuffer->SetData((uint32_t)sizeof(float) * 3, 4, _G.vertices.data(),
                           _G.vertices.size() * sizeof(float));

  _G.indexBuffer = new impl::IndexBuffer();
  _G.indexBuffer->SetData((uint32_t)_G.indices.size(), _G.indices.data(),
                          _G.indices.size() * sizeof(uint32_t));

  _G.vertexArray = new impl::VertexArray();
  _G.vertexArray->BindVertex(*_G.vertexBuffer);
  _G.vertexArray->BindIndex(*_G.indexBuffer);

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

  const std::vector<vk::ClearValue> clearValues = {
    vk::ClearValue().setColor(vk::ClearColorValue{1.0f, 0.2f, 0.2f, 1.0f}),
    vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue{1.0f, 0}),
  };

  for (uint32_t i = 0; i < _G.device->GetDrawCount(); i++) {
    auto dc = impl::DrawCmd(i, clearValues);
    dc.BindPipeline(*_G.basicPipe);
    dc.SetViewport();
    dc.SetScissor();
    dc.DrawVertex(*_G.vertexArray);
  }
}

void Application::OnLoop() {
  _G.device->Draw();
}

void Application::OnEnd() {
  delete _G.basicPipe;
  delete _G.vertexArray;
  delete _G.indexBuffer;
  delete _G.vertexBuffer;
  _G.device.reset();
  _G.window.reset();
}
} // namespace VPP
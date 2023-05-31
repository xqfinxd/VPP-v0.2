#include <iostream>

#include "VPP/application.h"
#include "shader_loader.h"

class MyApp : public VPP::Application {
 public:
  MyApp() {}
  ~MyApp() {}

  void OnLoop() override {}

 private:
};

int main(int argc, char** argv) {
  auto loader = VPP::impl::LoadShader({"texture.vert", "texture.frag"});
  MyApp app{};
  app.Run();
  VPP::impl::DestroyShader(loader);
}
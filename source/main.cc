#include <iostream>

#include "VPP/Application.h"

class MyApp : public VPP::Application {
public:
  MyApp() {
  }
  ~MyApp() {
  }

  void OnLoop() override {
    __super::OnLoop();
  }

private:
};

int main(int argc, char** argv) {
  MyApp app{};
  app.Run();
}
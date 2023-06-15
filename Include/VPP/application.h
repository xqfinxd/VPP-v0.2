#pragma once

#include "VPP_API.h"

namespace VPP {
class VPP_API Application {
 public:
  Application();
  ~Application();

  void Run();

 protected:
  virtual void OnStart();
  virtual void OnLoop();
  virtual void OnEnd();

 private:
};
}  // namespace VPP

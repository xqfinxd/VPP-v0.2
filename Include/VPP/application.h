#pragma once

#include "public/config.h"

namespace VPP {

class VPP_API Application {
public:
    Application();
    ~Application();

    void Run();

    virtual void OnStart();
    virtual void OnLoop();
    virtual void OnEnd();

private:
};

}  // namespace VPP
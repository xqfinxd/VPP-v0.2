#pragma once

#include "public/config.h"

namespace VPP {

class VPP_API Application {
public:
    Application();
    ~Application();

    void Run();

private:
};

}  // namespace VPP
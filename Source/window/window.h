#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <string>

#include "public/config.h"
#include "public/singleton.h"
#include "public/impl.h"

struct Window_D;
class VPP_API Window
    : public Singleton<Window>
    , public ImplBase<Window_D> {
public:
    struct Size {
        int width = 0, height = 0;
    };

    Window();
    ~Window();

    void setSize(int width, int height);
    void setFps(int fps);
    void setTitle(const char* title);
    void close();

    Size getArea();
    void run();
};

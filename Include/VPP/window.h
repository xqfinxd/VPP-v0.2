#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "public/config.h"
#include "public/impl.h"
#include "public/singleton.h"

struct Window_D;
class VPP_API Window : public Singleton<Window>, public ImplBase<Window_D> {
public:
    Window();
    ~Window();

    void setSize(int width, int height);
    void setFps(int fps);
    void setTitle(const char* title);
    void close();

    void run(std::function<void(Window&)> start,
             std::function<void(Window&)> loop,
             std::function<void(Window&)> end);
};

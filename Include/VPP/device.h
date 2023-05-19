#pragma once

#include "public/singleton.h"
#include "public/impl.h"
#include "public/config.h"
#include "window.h"

struct Renderer_D;

class VPP_API Renderer
    : public Singleton<Renderer>
    , public ImplBase<Renderer_D> {
public:
    Renderer();
    ~Renderer();

    void bindWindow(Window& window);
};

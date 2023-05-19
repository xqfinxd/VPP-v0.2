#include "VPP/device.h"
#include "VPP/window.h"

int main(int argc, char** argv) {
    Window win{};
    Renderer renderer{};
    win.setSize(1080, 600);
    win.setFps(60);
    win.setTitle("Sample");
    win.run([&renderer](Window& win) { renderer.bindWindow(win); }, {}, {});
}
#include "VPP/window.h"

int main(int argc, char** argv) {
    Window win{};
    win.setSize(1080, 600);
    win.setFps(60);
    win.setTitle("Sample");
    win.run();
}
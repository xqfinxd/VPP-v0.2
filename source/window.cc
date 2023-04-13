#include "window.h"
#include <cassert>
#include "config.h"

VkExtent2D MainWindow::GetSurfaceExtent() const {
    int w = 0, h = 0;
    glfwGetFramebufferSize(window_, &w, &h);
    return VkExtent2D{ (uint32_t)w, (uint32_t)h };
}

VkSurfaceKHR MainWindow::GetSurface(VkInstance instance) const {
    VkSurfaceKHR surface;
    auto res = glfwCreateWindowSurface(instance, window_, nullptr, &surface);
    assert(res == VK_SUCCESS);
    return surface;
}

std::vector<const char*> MainWindow::GetExtensions() const {
    std::vector<const char*> extensions{};
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    extensions.assign(glfwExtensions, glfwExtensions + glfwExtensionCount);

    return extensions;
}

void MainWindow::Run() {
    if (!window_)
        return;
    while (!glfwWindowShouldClose(window_)) {

        glfwPollEvents();
    }
}

MainWindow::MainWindow() {
    int res = GLFW_TRUE;
    res = glfwInit();
    assert(GLFW_TRUE == res);
    res = glfwVulkanSupported();
    assert(GLFW_TRUE == res);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto* pCfg = Config::Get();
    window_ = glfwCreateWindow(
        (int)pCfg->Integer("window.width"),
        (int)pCfg->Integer("window.height"),
        pCfg->String("window.title"),
        NULL, NULL
    );
    assert(window_);
}

MainWindow::~MainWindow() {
    glfwDestroyWindow(window_);
}

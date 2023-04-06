#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <cassert>
#include <memory>

#include "lua_ext.h"
#include "device.h"

struct WindowDeleter {
    void operator()(GLFWwindow* win) {
        if (win) {
            glfwDestroyWindow(win);
        }
    }
};
static std::unique_ptr<GLFWwindow, WindowDeleter> kUniqueWindow = nullptr;
extern GLFWwindow* GetWindow() { 
    if (kUniqueWindow)
        return kUniqueWindow.get();
    return nullptr;
}

static void InitGlfw() {
    assert(GLFW_TRUE == glfwInit() && "GLFW init failure!");
    assert(GLFW_TRUE == glfwVulkanSupported() && "Vulkan is not available!");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    kUniqueWindow = std::unique_ptr<GLFWwindow, WindowDeleter>{
        glfwCreateWindow(1080, 810, "vk", NULL, NULL)
    };
}

static int InitModule(lua_State* L) {
    printf("vk module init...\n");
    InitGlfw();
    GetDevice().Init();
    auto window = GetWindow();
    while (window && !glfwWindowShouldClose(window)) {

        glfwPollEvents();
    }
    return 0;
}

static int QuitModule(lua_State* L) {
    GetDevice().Quit();
    printf("vk module quit...\n");
    return 0;
}

static const luaL_Reg kLuaApi[] = {
    {"init", InitModule},
    {"quit", QuitModule},
    {NULL, NULL}
};

extern "C" {

    __declspec(dllexport) int luaopen_vk(lua_State* L) {
        luaL_newlib(L, kLuaApi);
        return 1;
    }

}

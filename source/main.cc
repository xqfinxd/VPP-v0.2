#include <cassert>
#include <memory>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "utility.h"
#include "device.h"
#include "window.h"
#include "config.h"

static int InitModule(lua_State* L) {
    printf("vk module init...\n");
    const char* fn = lua_tostring(L, 1);
    Config::Get()->LoadConfig(fn);
    MainWindow::Get();
    Renderer::Get();
    MainWindow::Get()->Run();
    return 0;
}

static int QuitModule(lua_State* L) {
    Renderer::Destroy();
    MainWindow::Destroy();
    Config::Destroy();
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
        lua_pushvalue(L, -1);
        lua_setglobal(L, "vk");
        return 1;
    }
}
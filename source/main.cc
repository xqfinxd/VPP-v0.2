#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <cassert>

#include "lua_ext.h"

static reg_object reg_window{};

static void init_glfw() {
    assert(GLFW_TRUE == glfwInit() && "GLFW init failure!");
    assert(GLFW_TRUE == glfwVulkanSupported() && "Vulkan is not available!");
}

static GLFWwindow* init_window(lua_State* L) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1080, 810, "vk", NULL, NULL);
    lua_pushlightuserdata(L, window);
    reg_window = reg_object(L, -1);
    lua_pop(L, 1);
    return window;
}

static void quit_window(lua_State* L) {
    auto window = (GLFWwindow*)reg_window.get(luaget::pointer{});
    glfwDestroyWindow(window);
    reg_window.dispose();
}

static int vk_init(lua_State* L) {
    printf("vk module init...\n");
    init_glfw();
    auto window = init_window(L);
    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();
    }
    return 0;
}

static int vk_quit(lua_State* L) {
    printf("vk module quit...\n");
    quit_window(L);
    return 0;
}

static const luaL_Reg vklib[] = {
  {"init", vk_init},
  {"quit", vk_quit},
  {NULL, NULL}
};

extern "C" {

    __declspec(dllexport) int luaopen_vk(lua_State* L) {
        luaL_newlib(L, vklib);
        return 1;
    }

}

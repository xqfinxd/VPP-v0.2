#include <lua.hpp>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <cassert>

static int vk_init(lua_State* L) {
    printf("vk module init...\n");
    assert(GLFW_TRUE == glfwInit() && "GLFW init failure!");
    assert(GLFW_TRUE == glfwVulkanSupported() && "Vulkan is not available!");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(640, 480, "vk", NULL, NULL);
    while (!glfwWindowShouldClose(window)) {
        
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    return 0;
}

static int vk_quit(lua_State* L) {
    printf("vk module quit...\n");
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

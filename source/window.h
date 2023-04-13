#pragma once
#include <mutex>
#include <vector>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "utility.h"

class MainWindow
	: public Singleton<MainWindow> {
public:
	MainWindow();
	~MainWindow();

	VkExtent2D GetSurfaceExtent() const;
	VkSurfaceKHR GetSurface(VkInstance instance) const;
	std::vector<const char*> GetExtensions() const;
	void Run();

protected:
	GLFWwindow* window_;

};

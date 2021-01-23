#include "graphicsWindow.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

GLFWwindow* s_pWindow = nullptr;
bool s_windowResized = false;

static void gameGraphicsWindowResizeCallback(GLFWwindow* pWindow, int width, int height)
{
	s_windowResized = true;
}

void pkGraphicsWindow_Create(const char* pWindowName)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	static const uint32_t WINDOW_WIDTH = 1920;
	static const uint32_t WINDOW_HEIGHT = 1080;

	s_pWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, pWindowName, nullptr, nullptr);
	glfwSetFramebufferSizeCallback(s_pWindow, gameGraphicsWindowResizeCallback);
}

void pkGraphicsWindow_Destroy()
{
	glfwDestroyWindow(s_pWindow);
}

GLFWwindow* pkGraphicsWindow_GetWindow()
{
    return s_pWindow;
}

bool pkGraphicsWindow_IsResized()
{
	return s_windowResized;
}

void pkGraphicsWindow_ResetResized()
{
	s_windowResized = false;
}

#include "game.h"

#include "graphics/graphics.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdio.h>

static const char* GAME_NAME = "zodquad";
static const uint32_t VERSION_MAJOR_NUMBER = 0;
static const uint32_t VERSION_MINOR_NUMBER = 0;
static const uint32_t VERSION_PATCH_NUMBER = 1;

static PkGraphicsWindow s_graphicsWindow;

static void gameGraphicsWindowResizeCallback(GLFWwindow* pWindow, int width, int height)
{
	s_graphicsWindow.windowResized = true;
}

static void gameGraphicsWindowInitialise()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	static const uint32_t WINDOW_WIDTH = 800;
	static const uint32_t WINDOW_HEIGHT = 600;

	char windowName[128];
	sprintf_s(windowName, "%s version %d.%d.%d", GAME_NAME, VERSION_MAJOR_NUMBER, VERSION_MINOR_NUMBER, VERSION_PATCH_NUMBER);

	s_graphicsWindow.pWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, windowName, nullptr, nullptr);
	glfwSetFramebufferSizeCallback(s_graphicsWindow.pWindow, gameGraphicsWindowResizeCallback);
}

static void gameGraphicsWindowCleanup()
{
	glfwDestroyWindow(s_graphicsWindow.pWindow);
	glfwTerminate();
}

static void gameInitialise()
{
	gameGraphicsWindowInitialise();
	PkGraphicsInitialise(s_graphicsWindow);
}

static void gameCleanup()
{
	PkGraphicsCleanup();
	gameGraphicsWindowCleanup();
}

static void gameUpdate()
{
	glfwPollEvents();
	PkGraphicsRender();
}

void PkGameMain()
{
	gameInitialise();

	while (!glfwWindowShouldClose(s_graphicsWindow.pWindow))
	{
		gameUpdate();
	}

	gameCleanup();
}

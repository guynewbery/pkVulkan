#include "game.h"

#include "graphics/pkGraphicsTest.h"
#include "camera/pkCamera.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <chrono>

static const char* GAME_NAME = "zodquad";
static const uint32_t VERSION_MAJOR_NUMBER = 0;
static const uint32_t VERSION_MINOR_NUMBER = 0;
static const uint32_t VERSION_PATCH_NUMBER = 1;

static PkGraphicsWindow s_graphicsWindow;
static PkGraphicsModelViewProjection s_graphicsModelViewProjection;

static std::chrono::time_point<std::chrono::high_resolution_clock> currentTime;
static std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;

static void gameGraphicsWindowResizeCallback(GLFWwindow* pWindow, int width, int height)
{
	s_graphicsWindow.windowResized = true;
}

static void gameGraphicsWindowInitialise()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	static const uint32_t WINDOW_WIDTH = 1920;
	static const uint32_t WINDOW_HEIGHT = 1080;

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

static void gameUpdate()
{
	lastTime = currentTime;
	currentTime = std::chrono::high_resolution_clock::now();
	float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();

	glfwPollEvents();

	pkCamera_Update(s_graphicsModelViewProjection, dt);
	pkGraphics_Render();
}

static void gameInitialise()
{
	currentTime = std::chrono::high_resolution_clock::now();

	gameGraphicsWindowInitialise();
	pkGraphics_Initialise(s_graphicsWindow, s_graphicsModelViewProjection);
//	graphicsImgui_Initialise(s_graphicsWindow.pWindow);
}

static void gameCleanup()
{
//	graphicsImgui_Cleanup();
	pkGraphics_Cleanup();
	gameGraphicsWindowCleanup();
}

void GameMain()
{
	gameInitialise();

	while (!glfwWindowShouldClose(s_graphicsWindow.pWindow))
	{
		gameUpdate();
	}

	gameCleanup();
}

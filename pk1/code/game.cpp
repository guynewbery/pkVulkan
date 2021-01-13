#include "game.h"

#include "graphics/pkGraphics.h"
#include "graphics/pkGraphicsTest.h"
#include "graphics/pkGraphicsWindow.h"
#include "camera/pkCamera.h"

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <chrono>

static const char* GAME_NAME = "zodquad";
static const uint32_t VERSION_MAJOR_NUMBER = 0;
static const uint32_t VERSION_MINOR_NUMBER = 0;
static const uint32_t VERSION_PATCH_NUMBER = 1;

static PkGraphicsModelViewProjection s_graphicsModelViewProjection;

static std::chrono::time_point<std::chrono::high_resolution_clock> currentTime;
static std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;

static void gameUpdate()
{
	lastTime = currentTime;
	currentTime = std::chrono::high_resolution_clock::now();
	float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();

	glfwPollEvents();

	pkCamera_Update(s_graphicsModelViewProjection, dt);

	pkGraphics_FrameRenderAndPresent();
}

static void onSwapChainCreate()
{
	pkGraphicsTest_OnSwapChainCreate();
}

static void onSwapChainDestroy()
{
	pkGraphicsTest_OnSwapChainDestroy();
}

static VkCommandBuffer& getCommandBuffer(uint32_t imageIndex)
{
	return pkGraphicsTest_GetCommandBuffer(imageIndex);
}

static void gameInitialise()
{
	currentTime = std::chrono::high_resolution_clock::now();

	glfwInit();

	char windowName[128];
	sprintf_s(windowName, "%s version %d.%d.%d", GAME_NAME, VERSION_MAJOR_NUMBER, VERSION_MINOR_NUMBER, VERSION_PATCH_NUMBER);
	pkGraphicsWindow_Create(windowName);

	pkGraphics_Initialise(s_graphicsModelViewProjection, onSwapChainCreate, onSwapChainDestroy, getCommandBuffer);

	pkGraphicsTest_Initialise();
}

static void gameCleanup()
{
	pkGraphics_WaitIdle();

	pkGraphicsTest_Cleanup();

	pkGraphics_Cleanup();

	pkGraphicsWindow_Destroy();

	glfwTerminate();
}

void GameMain()
{
	gameInitialise();

	while (!glfwWindowShouldClose(pkGraphicsWindow_GetWindow()))
	{
		gameUpdate();
	}

	gameCleanup();
}

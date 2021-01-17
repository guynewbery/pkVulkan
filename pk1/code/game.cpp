#include "game.h"

#include "graphics/graphicsImgui.h"
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

static void onSwapChainCreate()
{
	pkGraphicsTest_OnSwapChainCreate();
	graphicsImgui_OnSwapChainCreate();
}

static void onSwapChainDestroy()
{
	graphicsImgui_OnSwapChainDestroy();
	pkGraphicsTest_OnSwapChainDestroy();
}

static void getCommandBuffers(uint32_t imageIndex, std::vector<VkCommandBuffer>& buffers)
{
	buffers.resize(2);
	buffers[0] = pkGraphicsTest_GetCommandBuffer(imageIndex);
	buffers[1] = graphicsImgui_GetCommandBuffer(imageIndex);
}

static void gameUpdate()
{
	lastTime = currentTime;
	currentTime = std::chrono::high_resolution_clock::now();
	float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();

	glfwPollEvents();

	pkCamera_Update(s_graphicsModelViewProjection, dt);

	graphicsImgui_Update();

	pkGraphics_FrameRenderAndPresent();
}

static void gameInitialise()
{
	currentTime = std::chrono::high_resolution_clock::now();

	glfwInit();

	char windowName[128];
	sprintf_s(windowName, "%s version %d.%d.%d", GAME_NAME, VERSION_MAJOR_NUMBER, VERSION_MINOR_NUMBER, VERSION_PATCH_NUMBER);
	pkGraphicsWindow_Create(windowName);

	pkGraphics_Initialise(s_graphicsModelViewProjection, onSwapChainCreate, onSwapChainDestroy, getCommandBuffers);

	pkGraphicsTest_Initialise();
	graphicsImgui_Initialise();
}

static void gameCleanup()
{
	pkGraphics_WaitIdle();

	graphicsImgui_Cleanup();
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

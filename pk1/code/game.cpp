#include "game.h"

#include "library_macros.h"

#include "graphics/graphics.h"
#include "camera/camera.h"
#include "imgui/imgui.h"

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <chrono>

static const char* GAME_NAME = "zodquad";
static const uint32_t VERSION_MAJOR_NUMBER = 0;
static const uint32_t VERSION_MINOR_NUMBER = 0;
static const uint32_t VERSION_PATCH_NUMBER = 1;

struct GameData
{
	std::chrono::time_point<std::chrono::high_resolution_clock> currentTime;
	std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;
};

static GameData* s_pData = nullptr;

static void updateGame()
{
	PkGraphics::BeginImguiFrame();
	{
		s_pData->lastTime = s_pData->currentTime;
		s_pData->currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::seconds::period>(s_pData->currentTime - s_pData->lastTime).count();

		glfwPollEvents();

		//ImGui::ShowDemoWindow();
		pkCamera_Update(dt);
	}
	PkGraphics::EndImguiFrame();

	PkGraphics::RenderAndPresentFrame();
}

static void initialiseGame()
{
	s_pData = new GameData();

	char windowName[128];
	sprintf_s(windowName, "%s version %d.%d.%d", GAME_NAME, VERSION_MAJOR_NUMBER, VERSION_MINOR_NUMBER, VERSION_PATCH_NUMBER);

	s_pData->currentTime = std::chrono::high_resolution_clock::now();

	glfwInit();

	PkGraphics::InitialiseGraphics(windowName);
}

static void cleanupGame()
{
	PkGraphics::CleanupGraphics();

	glfwTerminate();

	delete s_pData;
}

void RunGame()
{
	initialiseGame();

	while (!PkGraphics::WindowShouldClose())
	{
		updateGame();
	}

	cleanupGame();
}

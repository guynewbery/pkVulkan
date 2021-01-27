#pragma once

#include <vulkan/vulkan_core.h>

#include <stdint.h>

class PkGraphicsRenderPassImgui
{
public:
	PkGraphicsRenderPassImgui() = delete;

	static VkCommandBuffer& GetCommandBuffer(const uint32_t imageIndex);

	static void BeginImguiFrame();
	static void EndImguiFrame();

	static void OnSwapChainCreate();
	static void OnSwapChainDestroy();

	static void InitialiseGraphicsRenderPassImgui();
	static void CleanupGraphicsRenderPassImgui();
};

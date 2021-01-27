#pragma once

#include <vulkan/vulkan_core.h>

#include <stdint.h>

class PkGraphicsRenderPassScene
{
public:
	PkGraphicsRenderPassScene() = delete;

	static VkCommandBuffer& GetCommandBuffer(uint32_t imageIndex);

	static void OnSwapChainCreate();
	static void OnSwapChainDestroy();

	static void InitialiseGraphicsRenderPassScene();
	static void CleanupGraphicsRenderPassScene();
};

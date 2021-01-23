#pragma once

#include <vulkan/vulkan_core.h>

#include <stdint.h>

VkCommandBuffer& pkGraphicsRenderPassImgui_GetCommandBuffer(uint32_t imageIndex);

void pkGraphicsRenderPassImgui_BeginFrame();
void pkGraphicsRenderPassImgui_EndFrame();

void pkGraphicsRenderPassImgui_OnSwapChainCreate();
void pkGraphicsRenderPassImgui_OnSwapChainDestroy();

void pkGraphicsRenderPassImgui_Initialise();
void pkGraphicsRenderPassImgui_Cleanup();

#pragma once

#include <vulkan/vulkan_core.h>

#include <stdint.h>

VkCommandBuffer& pkGraphicsRenderPassScene_GetCommandBuffer(uint32_t imageIndex);

void pkGraphicsRenderPassScene_OnSwapChainCreate();
void pkGraphicsRenderPassScene_OnSwapChainDestroy();

void pkGraphicsRenderPassScene_Initialise();
void pkGraphicsRenderPassScene_Cleanup();

#pragma once

#include <vulkan/vulkan_core.h>

#include <stdint.h>

VkCommandBuffer& pkGraphicsRenderPassMain_GetCommandBuffer(uint32_t imageIndex);

void pkGraphicsRenderPassMain_OnSwapChainCreate();
void pkGraphicsRenderPassMain_OnSwapChainDestroy();

void pkGraphicsRenderPassMain_Initialise();
void pkGraphicsRenderPassMain_Cleanup();

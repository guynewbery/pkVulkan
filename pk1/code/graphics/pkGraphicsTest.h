#pragma once

#include <vulkan/vulkan_core.h>

#include <stdint.h>

VkCommandBuffer& pkGraphicsTest_GetCommandBuffer(uint32_t imageIndex);

void pkGraphicsTest_OnSwapChainCreate();
void pkGraphicsTest_OnSwapChainDestroy();

void pkGraphicsTest_Initialise();
void pkGraphicsTest_Cleanup();
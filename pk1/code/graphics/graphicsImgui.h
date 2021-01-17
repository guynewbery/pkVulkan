#pragma once

#include <vulkan/vulkan_core.h>

#include <stdint.h>

VkCommandBuffer& graphicsImgui_GetCommandBuffer(uint32_t imageIndex);
void graphicsImgui_Update();

void graphicsImgui_Initialise();
void graphicsImgui_Cleanup();

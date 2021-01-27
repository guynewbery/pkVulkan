#pragma once

#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>

#include <stdint.h>

struct PkGraphicsModelViewProjection;

VkCommandBuffer& pkGraphicsRenderPassScene_GetCommandBuffer(uint32_t imageIndex);

void pkGraphicsRenderPassScene_OnSwapChainCreate();
void pkGraphicsRenderPassScene_OnSwapChainDestroy();

void pkGraphicsRenderPassScene_Initialise(PkGraphicsModelViewProjection& rModelViewProjection);
void pkGraphicsRenderPassScene_Cleanup();

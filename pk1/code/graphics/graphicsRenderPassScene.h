#pragma once

#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>

#include <stdint.h>

struct PkGraphicsModelViewProjection
{
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);

    float fieldOfView = 45.0f;
    float nearViewPlane = 1.0f;
    float farViewPlane = 100.0f;
};

VkCommandBuffer& pkGraphicsRenderPassScene_GetCommandBuffer(uint32_t imageIndex);

void pkGraphicsRenderPassScene_OnSwapChainCreate();
void pkGraphicsRenderPassScene_OnSwapChainDestroy();

void pkGraphicsRenderPassScene_Initialise(PkGraphicsModelViewProjection& rModelViewProjection);
void pkGraphicsRenderPassScene_Cleanup();

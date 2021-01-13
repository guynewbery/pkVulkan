#pragma once

#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>

struct PkGraphicsModelViewProjection
{
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);

    float fieldOfView = 45.0f;
    float nearViewPlane = 1.0f;
    float farViewPlane = 100.0f;
};

VkPhysicalDevice pkGraphics_GetPhysicalDevice();
VkDevice pkGraphics_GetDevice();
VkCommandPool pkGraphics_GetCommandPool();
VkDescriptorSetLayout* pkGraphics_GetDescriptorSetLayout();
VkQueue pkGraphics_GetGraphicsQueue();

void pkGraphics_GetPhysicalDeviceProperties(VkPhysicalDeviceProperties* physicalDeviceProperties);
void pkGraphics_GetFormatProperties(VkFormat imageFormat, VkFormatProperties* formatProperties);

VkSampleCountFlagBits pkGraphics_GetMaxMsaaSampleCount();

void pkGraphics_WaitIdle();
void pkGraphics_FrameRenderAndPresent();

void pkGraphics_Initialise(
    PkGraphicsModelViewProjection& rModelViewProjection, 
    void (*pOnSwapChainCreate)(), 
    void (*pOnSwapChainDestroy)(),
    VkCommandBuffer& (*pGetCommandBuffer)(uint32_t));

void pkGraphics_Cleanup();

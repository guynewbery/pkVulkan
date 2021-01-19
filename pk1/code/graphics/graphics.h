#pragma once

#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>
#include <vector>

VkInstance pkGraphics_GetInstance();
VkPhysicalDevice pkGraphics_GetPhysicalDevice();
VkDevice pkGraphics_GetDevice();
VkCommandPool pkGraphics_GetCommandPool();
VkDescriptorSetLayout* pkGraphics_GetDescriptorSetLayout();
VkQueue pkGraphics_GetGraphicsQueue();

void pkGraphics_GetPhysicalDeviceProperties(VkPhysicalDeviceProperties* physicalDeviceProperties);
void pkGraphics_GetFormatProperties(VkFormat imageFormat, VkFormatProperties* formatProperties);

VkSampleCountFlagBits pkGraphics_GetMaxMsaaSampleCount();

void pkGraphics_WaitIdle();
void pkGraphics_RenderAndPresentFrame();

void pkGraphics_Initialise(
    void (*pOnSwapChainCreate)(), 
    void (*pOnSwapChainDestroy)(),
    void (*pGetCommandBuffers)(uint32_t, std::vector<VkCommandBuffer>&));

void pkGraphics_Cleanup();

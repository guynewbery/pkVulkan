#pragma once

#include <vulkan/vulkan_core.h>

#include <vk_mem_alloc.h>

VmaAllocator pkGraphics_GetAllocator();
VkInstance pkGraphics_GetInstance();
VkPhysicalDevice pkGraphics_GetPhysicalDevice();
VkDevice pkGraphics_GetDevice();
VkCommandPool pkGraphics_GetCommandPool();

VkQueue pkGraphics_GetGraphicsQueue();
VkQueue pkGraphics_GetPresentQueue();

void pkGraphics_GetPhysicalDeviceProperties(VkPhysicalDeviceProperties* physicalDeviceProperties);
void pkGraphics_GetFormatProperties(VkFormat imageFormat, VkFormatProperties* formatProperties);

VkSampleCountFlagBits pkGraphics_GetMaxMsaaSampleCount();

void pkGraphics_Initialise();
void pkGraphics_Cleanup();

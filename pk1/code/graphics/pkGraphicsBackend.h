#pragma once

#include <vulkan/vulkan_core.h>

#include <vk_mem_alloc.h>

#include <vector>

struct GLFWwindow;

VkFormat pkGraphicsBackend_FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

VmaAllocator pkGraphicsBackend_GetAllocator();
VkInstance pkGraphicsBackend_GetInstance();
VkPhysicalDevice pkGraphicsBackend_GetPhysicalDevice();
VkDevice pkGraphicsBackend_GetDevice();
VkCommandPool pkGraphicsBackend_GetCommandPool();

VkQueue pkGraphicsBackend_GetGraphicsQueue();
VkQueue pkGraphicsBackend_GetPresentQueue();

void pkGraphicsBackend_GetPhysicalDeviceProperties(VkPhysicalDeviceProperties* physicalDeviceProperties);
void pkGraphicsBackend_GetFormatProperties(VkFormat imageFormat, VkFormatProperties* formatProperties);

VkSampleCountFlagBits pkGraphicsBackend_GetMaxMsaaSampleCount();

void pkGraphicsBackend_Initialise();
void pkGraphicsBackend_Cleanup();

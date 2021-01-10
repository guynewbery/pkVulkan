#pragma once

#include <vulkan/vulkan_core.h>

#include <vk_mem_alloc.h>

#include <vector>
#include <optional>

struct GLFWwindow;

struct PkGraphicsBackendSwapChainSupport 
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct PkGraphicsBackendQueueFamilyIndices 
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() 
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

VkDevice pkGraphicsBackend_Initialise(GLFWwindow& rWindow);
void pkGraphicsBackend_Cleanup();

PkGraphicsBackendSwapChainSupport pkGraphicsBackend_QuerySwapChainSupport(VkPhysicalDevice physicalDevice);
PkGraphicsBackendQueueFamilyIndices pkGraphicsBackend_FindQueueFamilies(VkPhysicalDevice physicalDevice);
VkFormat pkGraphicsBackend_FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

VmaAllocator pkGraphicsBackend_GetAllocator();
VkSurfaceKHR pkGraphicsBackend_GetSurface();
VkPhysicalDevice pkGraphicsBackend_GetPhysicalDevice();
VkSampleCountFlagBits pkGraphicsBackend_GetMaxMsaaSampleCount();

void pkGraphicsBackend_GetPhysicalDeviceProperties(VkPhysicalDeviceProperties* physicalDeviceProperties);
void pkGraphicsBackend_GetFormatProperties(VkFormat imageFormat, VkFormatProperties* formatProperties);

VkQueue pkGraphicsBackend_GetGraphicsQueue();
VkQueue pkGraphicsBackend_GetPresentQueue();

#pragma once

#include <vulkan/vulkan_core.h>

#include <vector>

class PkGraphicsCore
{
public:
    PkGraphicsCore() = delete;

    static VkInstance GetInstance();
    static VkSurfaceKHR GetSurface();
    static VkPhysicalDevice GetPhysicalDevice();
    static VkDevice GetDevice();

    static VkQueue GetGraphicsQueue();
    static VkQueue GetPresentQueue();

    static void GetPhysicalDeviceProperties(VkPhysicalDeviceProperties* physicalDeviceProperties);
    static void GetFormatProperties(VkFormat imageFormat, VkFormatProperties* formatProperties);

    static VkSampleCountFlagBits GetMaxMsaaSampleCount();

    static void InitialiseGraphicsCore();
    static void CleanupGraphicsCore();
};

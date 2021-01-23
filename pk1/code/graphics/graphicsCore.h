#pragma once

#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>

class PkGraphicsCore
{
public:
    PkGraphicsCore() = delete;

    static VkInstance GetInstance();
    static VkSurfaceKHR GetSurface();
    static VkPhysicalDevice GetPhysicalDevice();
    static VkDevice GetDevice();
    static VmaAllocator GetAllocator();

    static VkQueue GetGraphicsQueue();
    static VkQueue GetPresentQueue();

    static void GetPhysicalDeviceProperties(VkPhysicalDeviceProperties* physicalDeviceProperties);
    static void GetFormatProperties(VkFormat imageFormat, VkFormatProperties* formatProperties);

    static VkSampleCountFlagBits GetMaxMsaaSampleCount();

    static void InitialiseGraphicsCore();
    static void CleanupGraphicsCore();
};

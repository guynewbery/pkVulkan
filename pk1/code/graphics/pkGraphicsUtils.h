#pragma once

#include <vulkan/vulkan_core.h>

#include <vector>
#include <optional>

struct PkGraphicsUtilsSwapChainSupport
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

PkGraphicsUtilsSwapChainSupport pkGraphicsUtils_QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

struct PkGraphicsUtilsQueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

PkGraphicsUtilsQueueFamilyIndices pkGraphicsUtils_FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

VkImageView pkGraphicsUtils_CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

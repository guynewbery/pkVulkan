#pragma once

#include <vulkan/vulkan_core.h>

#include <vector>
#include <optional>

struct PkGraphicsSwapChainSupport
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


struct PkGraphicsQueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class PkGraphicsUtils
{
public:
    PkGraphicsUtils() = delete;

    static PkGraphicsSwapChainSupport QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    static PkGraphicsQueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    static VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

    static VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
    static void EndSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer);
};

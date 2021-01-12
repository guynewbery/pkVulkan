#pragma once

#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>

#include <vector>
#include <optional>

void PkGraphicsUtils_CreateBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* pBuffer, VmaAllocation* pBufferAllocation);

struct PkGraphicsSwapChainSupport
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

PkGraphicsSwapChainSupport pkGraphicsUtils_QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

struct PkGraphicsQueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

PkGraphicsQueueFamilyIndices pkGraphicsUtils_FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

VkImageView pkGraphicsUtils_CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

VkFormat pkGraphicsUtils_FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

VkFormat pkGraphicsUtils_FindDepthFormat(VkPhysicalDevice physicalDevice);

VkCommandBuffer pkGraphicsUtils_BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);

void pkGraphicsUtils_EndSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer);

void pkGraphicsUtils_CreateImage(VkDevice device, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageAspectFlags aspectFlags, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkImageView& imageView, VmaAllocation& imageAllocation);

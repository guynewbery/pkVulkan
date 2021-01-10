#include "pkGraphicsSwapChain.h"

#include "graphics/pkGraphicsUtils.h"
#include "graphics/pkGraphicsSurface.h"
#include "graphics/pkGraphicsWindow.h"
#include "graphics/pkGraphicsAllocator.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <array>

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(pkGraphicsWindow_GetWindow(), &width, &height);

        VkExtent2D actualExtent =
        {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void pkGraphicsSwapChain_Create(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, PkGraphicsSwapChain& rSwapChain)
{
    PkGraphicsSwapChainSupport swapChainSupport = pkGraphicsUtils_QuerySwapChainSupport(physicalDevice, pkGraphicsSurface_GetSurface());

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = pkGraphicsSurface_GetSurface();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    PkGraphicsQueueFamilyIndices indices = pkGraphicsUtils_FindQueueFamilies(physicalDevice, pkGraphicsSurface_GetSurface());
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &rSwapChain.swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, rSwapChain.swapChain, &imageCount, nullptr);
    rSwapChain.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, rSwapChain.swapChain, &imageCount, rSwapChain.swapChainImages.data());

    rSwapChain.swapChainImageFormat = surfaceFormat.format;
    rSwapChain.swapChainExtent = extent;

    rSwapChain.swapChainImageViews.resize(rSwapChain.swapChainImages.size());
    for (uint32_t i = 0; i < rSwapChain.swapChainImages.size(); i++)
    {
        rSwapChain.swapChainImageViews[i] = pkGraphicsUtils_CreateImageView(device, rSwapChain.swapChainImages[i], rSwapChain.swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    rSwapChain.uniformBuffers.resize(rSwapChain.swapChainImages.size());
    rSwapChain.uniformBufferAllocations.resize(rSwapChain.swapChainImages.size());
    for (size_t i = 0; i < rSwapChain.swapChainImages.size(); i++)
    {
        PkGraphicsUtils_CreateBuffer(pkGraphicsAllocator_GetAllocator(), bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &rSwapChain.uniformBuffers[i], &rSwapChain.uniformBufferAllocations[i]);
    }

    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(rSwapChain.swapChainImages.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(rSwapChain.swapChainImages.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(rSwapChain.swapChainImages.size());

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &rSwapChain.descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void pkGraphicsSwapChain_Destroy(VkDevice device, PkGraphicsSwapChain& rSwapChain)
{
    vkDestroyDescriptorPool(device, rSwapChain.descriptorPool, nullptr);

    for (size_t i = 0; i < rSwapChain.swapChainImages.size(); i++)
    {
        vmaDestroyBuffer(pkGraphicsAllocator_GetAllocator(), rSwapChain.uniformBuffers[i], rSwapChain.uniformBufferAllocations[i]);
    }

    for (VkImageView imageView : rSwapChain.swapChainImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, rSwapChain.swapChain, nullptr);
}

/*
int                 Width;
int                 Height;
VkSwapchainKHR      Swapchain;
VkSurfaceKHR        Surface;
VkSurfaceFormatKHR  SurfaceFormat;
VkPresentModeKHR    PresentMode;
VkRenderPass        RenderPass;
VkPipeline          Pipeline;               // The window pipeline may uses a different VkRenderPass than the one passed in ImGui_ImplVulkan_InitInfo
bool                ClearEnable;
VkClearValue        ClearValue;
uint32_t            FrameIndex;             // Current frame being rendered to (0 <= FrameIndex < FrameInFlightCount)
uint32_t            ImageCount;             // Number of simultaneous in-flight frames (returned by vkGetSwapchainImagesKHR, usually derived from min_image_count)
uint32_t            SemaphoreIndex;         // Current set of swapchain wait semaphores we're using (needs to be distinct from per frame data)
ImGui_ImplVulkanH_Frame* Frames;
ImGui_ImplVulkanH_FrameSemaphores* FrameSemaphores;
*/

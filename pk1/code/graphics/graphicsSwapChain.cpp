#include "graphicsSwapChain.h"

#include "graphics/graphicsAllocator.h"
#include "graphics/graphicsCore.h"
#include "graphics/graphicsUtils.h"
#include "graphics/graphicsWindow.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <array>

static PkGraphicsSwapChain s_swapChain;

PkGraphicsSwapChain pkGraphicsSwapChain_GetSwapChain()
{
    return s_swapChain;
}

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

void pkGraphicsSwapChain_Create(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VkSampleCountFlagBits maxMsaaSampleCount)
{
    PkGraphicsSwapChainSupport swapChainSupport = pkGraphicsUtils_QuerySwapChainSupport(physicalDevice, PkGraphicsCore::GetSurface());

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
    createInfo.surface = PkGraphicsCore::GetSurface();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    PkGraphicsQueueFamilyIndices indices = pkGraphicsUtils_FindQueueFamilies(physicalDevice, PkGraphicsCore::GetSurface());
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

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &s_swapChain.swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, s_swapChain.swapChain, &imageCount, nullptr);
    s_swapChain.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, s_swapChain.swapChain, &imageCount, s_swapChain.swapChainImages.data());

    s_swapChain.swapChainImageFormat = surfaceFormat.format;
    s_swapChain.swapChainExtent = extent;

    s_swapChain.swapChainImageViews.resize(s_swapChain.swapChainImages.size());
    for (uint32_t i = 0; i < s_swapChain.swapChainImages.size(); i++)
    {
        s_swapChain.swapChainImageViews[i] = pkGraphicsUtils_CreateImageView(device, s_swapChain.swapChainImages[i], s_swapChain.swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void pkGraphicsSwapChain_Destroy(VkDevice device)
{
    for (VkImageView imageView : s_swapChain.swapChainImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, s_swapChain.swapChain, nullptr);
}

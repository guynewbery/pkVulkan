#include "graphicsSwapChain.h"

#include "graphics/graphicsCore.h"
#include "graphics/graphicsUtils.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <array>

struct PkGraphicsSwapChainData
{
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkExtent2D swapChainExtent{ 0,0 };

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
};

static PkGraphicsSwapChainData* s_pData = nullptr;

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
        glfwGetFramebufferSize(PkGraphicsCore::GetWindow(), &width, &height);

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

/*static*/ VkSwapchainKHR PkGraphicsSwapChain::GetSwapChain()
{
    return s_pData->swapChain;
}

/*static*/ VkExtent2D PkGraphicsSwapChain::GetSwapChainExtent()
{
    return s_pData->swapChainExtent;
}

/*static*/ uint32_t PkGraphicsSwapChain::GetNumSwapChainImages()
{
    return static_cast<uint32_t>(s_pData->swapChainImageViews.size());
}

/*static*/ VkImageView PkGraphicsSwapChain::GetSwapChainImageView(const uint32_t imageIndex)
{
    return s_pData->swapChainImageViews[imageIndex];
}

/*static*/ VkFormat PkGraphicsSwapChain::GetSwapChainImageFormat()
{
    return s_pData->swapChainImageFormat;
}

/*static*/ void PkGraphicsSwapChain::InitialiseGraphicsSwapChain()
{
    s_pData = new PkGraphicsSwapChainData();

    PkGraphicsSwapChainSupport swapChainSupport = pkGraphicsUtils_QuerySwapChainSupport(PkGraphicsCore::GetPhysicalDevice(), PkGraphicsCore::GetSurface());

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

    PkGraphicsQueueFamilyIndices indices = pkGraphicsUtils_FindQueueFamilies(PkGraphicsCore::GetPhysicalDevice(), PkGraphicsCore::GetSurface());
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

    if (vkCreateSwapchainKHR(PkGraphicsCore::GetDevice(), &createInfo, nullptr, &s_pData->swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(PkGraphicsCore::GetDevice(), s_pData->swapChain, &imageCount, nullptr);
    s_pData->swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(PkGraphicsCore::GetDevice(), s_pData->swapChain, &imageCount, s_pData->swapChainImages.data());

    s_pData->swapChainImageFormat = surfaceFormat.format;
    s_pData->swapChainExtent = extent;

    s_pData->swapChainImageViews.resize(s_pData->swapChainImages.size());
    for (uint32_t i = 0; i < s_pData->swapChainImages.size(); i++)
    {
        s_pData->swapChainImageViews[i] = pkGraphicsUtils_CreateImageView(PkGraphicsCore::GetDevice(), s_pData->swapChainImages[i], s_pData->swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

/*static*/ void PkGraphicsSwapChain::CleanupGraphicsSwapChain()
{
    for (VkImageView imageView : s_pData->swapChainImageViews)
    {
        vkDestroyImageView(PkGraphicsCore::GetDevice(), imageView, nullptr);
    }

    vkDestroySwapchainKHR(PkGraphicsCore::GetDevice(), s_pData->swapChain, nullptr);

    delete s_pData;
}

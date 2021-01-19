#pragma once

#include <vulkan/vulkan_core.h>

#include <vector>

struct PkGraphicsSwapChain
{
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D swapChainExtent{ 0,0 };

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
};

PkGraphicsSwapChain pkGraphicsSwapChain_GetSwapChain();

void pkGraphicsSwapChain_Create(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VkSampleCountFlagBits maxMsaaSampleCount);
void pkGraphicsSwapChain_Destroy(VkDevice device);
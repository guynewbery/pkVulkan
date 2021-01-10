#pragma once

#include <vulkan/vulkan_core.h>

#include <vector>

struct GLFWwindow;

struct PkGraphicsSwapChain
{
	int width;
	int height;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
};

void pkGraphicsSwapChain_Create(PkGraphicsSwapChain& rSwapChain, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, GLFWwindow* pWindow);
void pkGraphicsSwapChain_Destroy(PkGraphicsSwapChain& rSwapChain, VkDevice device);

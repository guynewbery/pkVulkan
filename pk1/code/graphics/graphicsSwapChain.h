#pragma once

#include <vulkan/vulkan_core.h>

class PkGraphicsSwapChain
{
public:
	PkGraphicsSwapChain() = delete;

	static VkSwapchainKHR GetSwapChain();
	static VkExtent2D GetSwapChainExtent();

	static uint32_t GetNumSwapChainImages();
	static VkImageView GetSwapChainImageView(const uint32_t imageIndex);
	static VkFormat GetSwapChainImageFormat();

	static void InitialiseGraphicsSwapChain();
	static void CleanupGraphicsSwapChain();
};

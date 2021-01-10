#pragma once

#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

#include <vector>

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

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

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VmaAllocation> uniformBufferAllocations;
};

void pkGraphicsSwapChain_Create(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, PkGraphicsSwapChain& rSwapChain);
void pkGraphicsSwapChain_Destroy(VkDevice device, PkGraphicsSwapChain& rSwapChain);

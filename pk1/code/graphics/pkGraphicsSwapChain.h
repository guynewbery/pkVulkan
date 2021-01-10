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
	int width = 0;
	int height = 0;

	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D swapChainExtent{ 0,0 };

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VmaAllocation> uniformBufferAllocations;

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
};

void pkGraphicsSwapChain_Create(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, PkGraphicsSwapChain& rSwapChain);
void pkGraphicsSwapChain_Destroy(VkDevice device, PkGraphicsSwapChain& rSwapChain);

#pragma once

#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>

VmaAllocator pkGraphicsAllocator_GetAllocator();

void pkGraphicsAllocator_Create(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
void pkGraphicsAllocator_Destroy();

#include "pkGraphicsAllocator.h"

#include <iostream>

VmaAllocator s_allocator = VK_NULL_HANDLE;

VmaAllocator pkGraphicsAllocator_GetAllocator()
{
    return s_allocator;
}

void pkGraphicsAllocator_Create(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;

    if (vmaCreateAllocator(&allocatorInfo, &s_allocator) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create allocator!");
    }
}

void pkGraphicsAllocator_Destroy()
{
    vmaDestroyAllocator(s_allocator);
}

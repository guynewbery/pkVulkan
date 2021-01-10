#pragma once

#include <vulkan/vulkan_core.h>

VkSurfaceKHR pkGraphicsSurface_GetSurface();

void pkGraphicsSurface_Create(VkInstance instance);
void pkGraphicsSurface_Destroy(VkInstance instance);

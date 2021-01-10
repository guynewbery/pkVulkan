#pragma once

#include <vulkan/vulkan_core.h>

void pkGraphicsSurface_Create(VkInstance instance);
void pkGraphicsSurface_Destroy(VkInstance instance);
VkSurfaceKHR pkGraphicsSurface_GetSurface();

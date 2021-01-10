#pragma once

#include <vulkan/vulkan_core.h>

struct GLFWwindow;

void pkGraphicsSurface_Create(VkInstance instance, GLFWwindow* pWindow);
void pkGraphicsSurface_Destroy(VkInstance instance);
VkSurfaceKHR pkGraphicsSurface_GetSurface();

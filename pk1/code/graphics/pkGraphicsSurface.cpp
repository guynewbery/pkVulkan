#include "pkGraphicsSurface.h"

#include "graphics/pkGraphicsWindow.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

VkSurfaceKHR s_surface;

void pkGraphicsSurface_Create(VkInstance instance)
{
    if (glfwCreateWindowSurface(instance, pkGraphicsWindow_GetWindow(), nullptr, &s_surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
}

void pkGraphicsSurface_Destroy(VkInstance instance)
{
    vkDestroySurfaceKHR(instance, s_surface, nullptr);
}

VkSurfaceKHR pkGraphicsSurface_GetSurface()
{
    return s_surface;
}

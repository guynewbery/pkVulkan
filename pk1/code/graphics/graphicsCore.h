#pragma once

#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

struct GLFWwindow;

class PkGraphicsCore
{
public:
    PkGraphicsCore() = delete;

    static GLFWwindow* GetWindow();
    static bool HasWindowBeenResized();
    static void ResetWindowResizedFlag();

    static VkInstance GetInstance();
    static VkSurfaceKHR GetSurface();
    static VkPhysicalDevice GetPhysicalDevice();
    static VkDevice GetDevice();
    static VmaAllocator GetAllocator();
    static VkCommandPool GetCommandPool();

    static VkQueue GetGraphicsQueue();
    static VkQueue GetPresentQueue();

    static void GetPhysicalDeviceProperties(VkPhysicalDeviceProperties* physicalDeviceProperties);
    static void GetFormatProperties(VkFormat imageFormat, VkFormatProperties* formatProperties);

    static VkSampleCountFlagBits GetMaxMsaaSampleCount();

    static glm::mat4& GetViewMatrix();
    static void SetViewMatrix(const glm::mat4& rMat);

    static float GetFieldOfView();
    static void SetFieldOfView(const float fov);

    static float GetNearViewPlane();
    static float GetFarViewPlane();

    static void InitialiseGraphicsCore(const char* pWindowName);
    static void CleanupGraphicsCore();
};

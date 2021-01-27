#pragma once

#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>

class PkGraphics
{
public:
    PkGraphics() = delete;

    static bool WindowShouldClose();
    static void RenderAndPresentFrame();

    static void BeginImguiFrame();
    static void EndImguiFrame();

    static void SetViewMatrix(const glm::mat4& rMat);
    static void SetFieldOfView(const float fov);

    static void InitialiseGraphics(const char* pWindowName);
    static void CleanupGraphics();
};

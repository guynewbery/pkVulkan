#pragma once

#include <vulkan/vulkan_core.h>

#include <vector>

struct PkGraphicsModelViewProjection;

class PkGraphics
{
public:
    PkGraphics() = delete;

    static bool WindowShouldClose();
    static void RenderAndPresentFrame();

    static void InitialiseGraphics(const char* pWindowName, PkGraphicsModelViewProjection& rModelViewProjection);
    static void CleanupGraphics();
};

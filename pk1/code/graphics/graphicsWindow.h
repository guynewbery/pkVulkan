#pragma once

struct GLFWwindow;

GLFWwindow* pkGraphicsWindow_GetWindow();

bool pkGraphicsWindow_IsResized();
void pkGraphicsWindow_ResetResized();

void pkGraphicsWindow_Create(const char* pWindowName);
void pkGraphicsWindow_Destroy();

#include "camera.h"

#include "graphics/graphicsModel.h"
#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>

static float s_cameraRotation = 75.0f;
static float s_camearLookAtEyeY = 1.6f;
static float s_camearLookAtEyeZ = 0.8f;
static float s_camearLookAtCentreZ = 0.2f;
static float s_camearFieldOfView = 45.0f;

static void ShowCameraUi(bool* p_open)
{
    const float DISTANCE = 10.0f;
    static int corner = 0;
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (corner != -1)
    {
        window_flags |= ImGuiWindowFlags_NoMove;
        ImVec2 window_pos = ImVec2((corner & 1) ? io.DisplaySize.x - DISTANCE : DISTANCE, (corner & 2) ? io.DisplaySize.y - DISTANCE : DISTANCE);
        ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    }
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background

    if (ImGui::Begin("Example: Simple overlay", p_open, window_flags))
    {
        static ImGuiSliderFlags sliderFlags = ImGuiSliderFlags_None;

        ImGui::SliderFloat("Camera look-at Eye Y", &s_camearLookAtEyeY, 0.0f, 3.0f, "%.3f", sliderFlags);
        ImGui::SliderFloat("Camera look-at Eye Z", &s_camearLookAtEyeZ, 0.0f, 3.0f, "%.3f", sliderFlags);
        ImGui::SliderFloat("Camera look-at Centre Y", &s_camearLookAtCentreZ, 0.0f, 3.0f, "%.3f", sliderFlags);
        ImGui::SliderFloat("Camera field of view", &s_camearFieldOfView, 0.0f, 100.0f, "%.3f", sliderFlags);
        ImGui::SliderFloat("Camera rotation", &s_cameraRotation, 0.0f, 360.0f, "%.3f", sliderFlags);
    }
    ImGui::End();
}

void pkCamera_Update(PkGraphicsModelViewProjection& rModelViewProjection, float dt)
{
	//s_cameraRotation += dt * 10.0f;

    static bool show_camera_ui = true;
    ShowCameraUi(&show_camera_ui);

	rModelViewProjection.model = glm::rotate(glm::mat4(1.0f), glm::radians(s_cameraRotation), glm::vec3(0.0f, 0.0f, 1.0f));
	rModelViewProjection.view = glm::lookAt(glm::vec3(0.0f, s_camearLookAtEyeY, s_camearLookAtEyeZ), glm::vec3(0.0f, 0.0f, s_camearLookAtCentreZ), glm::vec3(0.0f, 0.0f, 1.0f));
	rModelViewProjection.fieldOfView = s_camearFieldOfView;
}

#include <glm/glm.hpp>

struct GLFWwindow;

struct PkGraphicsWindow
{
    GLFWwindow* pWindow = nullptr;
    bool windowResized = false;
};

struct PkGraphicsModelViewProjection
{
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);

    float fieldOfView = 45.0f;
    float nearViewPlane = 1.0f;
    float farViewPlane = 10.0f;
};

void PkGraphicsInitialise(PkGraphicsWindow& rGraphicsWindow, PkGraphicsModelViewProjection& rModelViewProjection);
void PkGraphicsCleanup();

void PkGraphicsUpdateModelViewProjection(const glm::mat4& rModel, const glm::mat4& rView, const float fieldOfView);
void PkGraphicsRender();

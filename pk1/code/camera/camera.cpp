#include "camera.h"

#include "graphics/graphics.h"

#include <glm/gtc/matrix_transform.hpp>

static float s_cameraRotation = 0.0f;

void PkCameraUpdate(PkGraphicsModelViewProjection& rModelViewProjection, float dt)
{
	s_cameraRotation += dt * 90.0f;

	rModelViewProjection.model = glm::rotate(glm::mat4(1.0f), glm::radians(s_cameraRotation), glm::vec3(0.0f, 0.0f, 1.0f));
	rModelViewProjection.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	rModelViewProjection.fieldOfView = 45.0f;
}

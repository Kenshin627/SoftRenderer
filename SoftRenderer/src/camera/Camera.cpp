#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up):
	position(eye),
	view(glm::identity<glm::mat4>()), 
	projection(glm::identity<glm::mat4>()),
	viewProjection(glm::identity<glm::mat4>())
{
	glm::vec3 z = glm::normalize(eye - center);
	glm::vec3 y = glm::normalize(up);
	glm::vec3 x = glm::normalize(glm::cross(y, z));
	y			= glm::normalize(glm::cross(z, x));

	//rotation
	glm::mat4 cameraRotation = {
		{ x.x, x.y, x.z, 0 },
		{ y.x, y.y, y.z, 0 },
		{ z.x, z.y, z.z, 0 },
		{ 0  , 0  , 0  , 1 }
	};

	//translation
	glm::mat4 cameraTranlation = {
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ eye.x, eye.y, eye.z, 1 }
	};
	
	//TODO: view matrix = cameraTranlation * cameraRotation)^-1 = cameraRotation^-1 * cameraTranslation^-1 = cameraRotation^T * cameraTranslation^-1
	glm::mat4 cameraRotationInvert = {
		{ x.x, y.x, z.x, 0 },
		{ x.y, y.y, z.y, 0 },
		{ x.z, y.z, z.z, 0 },
		{ 0  , 0  , 0  , 1 }
	};
	glm::mat4 cameraTranslationInvert = {
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ -eye.x, -eye.y, -eye.z, 1 }
	};
	view = cameraRotationInvert * cameraTranslationInvert;
}

void Camera::Perspective(float fov, float aspect, float near, float far)
{
	projection = glm::perspective(fov, aspect, near, far);
	viewProjection = projection * view;
}

void Camera::OrthoGraphic(float left, float right, float bottom, float top, float near, float far)
{
	projection = glm::ortho(left, right, bottom, top, near, far);
	viewProjection = projection * view;
}
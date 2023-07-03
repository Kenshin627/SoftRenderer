#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, float fov, float aspectRatio, float near, float far)
{
	//Rotation
	glm::vec3 z = eye - center;
	z = glm::normalize(z);
	glm::vec3 x = glm::cross(up, z);
	x = glm::normalize(x);
	glm::vec3 y = glm::cross(z, x);
	y = glm::normalize(y);

	/*
	 * |                |
	 * |   X��          |
	 * |   Y��          |
	 * |   Z��          |
	 */
	glm::mat4 rotation =
	{
		{ x.x, x.y, x.z, 0 },
		{ y.x, y.y, y.z, 0 },
		{ z.x, z.y, z.z, 0 },
		{ 0  ,   0,   0, 1 }
	};

	//Translation
	glm::mat4 translation =
	{
		{ 1,         0,     0, 0 },
		{ 0,         1,     0, 0 },
		{ 0,         0,     1, 0 },
		{ eye.x, eye.y, eye.z, 1 }
	};
	
	//view = glm::inverse(translation * rotation) = glm::inverse(rotation) * glm::inverse(translation) = glm::transpose(rotation) * glm::inverse(translation);
	glm::mat4 rotationInvert = 
	{
		{ x.x, y.x, z.x, 0 },
		{ x.y, y.y, z.y, 0 },
		{ x.z, y.z, z.z, 0 },
		{ 0  ,   0,   0, 1 }
	};

	glm::mat4 translationInvert =
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ -eye.x, -eye.y, -eye.z, 1 }
	};

	view = rotationInvert * translationInvert;
	projection = glm::perspective(fov, aspectRatio, near, far);
}

void Camera::Update()
{

}
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, float fov, float aspectRatio, float near, float far)
{
	zNear = near;
	zFar = far;

	//Rotation
	glm::vec3 z = eye - center;
	z = glm::normalize(z);
	glm::vec3 x = glm::cross(up, z);
	x = glm::normalize(x);
	glm::vec3 y = glm::cross(z, x);
	y = glm::normalize(y);

	/*
	 * |                |
	 * |   X÷·          |
	 * |   Y÷·          |
	 * |   Z÷·          |
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
	//projection = glm::perspective(fov, aspectRatio, near, far);
	//1:º∑—π±‰ªª
	glm::mat4 extrustionFrustum =
	{
		{ near, 0, 0, 0 },
		{ 0, near, 0, 0 },
		{ 0, 0, near + far, 1 },
		{ 0, 0, -near * far, 0 }
	};
	//2.∆Ω“∆£∫
	glm::mat4 translationFrustum =
	{
		{1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, (near + far) / 2.0f, 1 }
	};
	//3.Àı∑≈£∫
	float halfH = tan(fov / 2.0) * near;
	float halfW = halfH * aspectRatio;

	glm::mat4 scaleFrustum =
	{
		{ 1.0 / halfW, 0, 0, 0 },
		{ 0, 1.0 / halfH, 0, 0 },
		{ 0, 0, (far - near) / 2.0f, 0.0 },
		{ 0, 0, 0, 1 }
	};
	//projection = scaleFrustum * translationFrustum * extrustionFrustum;
	projection = glm::perspective(fov, aspectRatio, near, far);
	//glm::mat4 pro2 = glm::perspective(fov, aspectRatio, near, far);
	viewProjection = projection * view;
}

void Camera::Update()
{

}
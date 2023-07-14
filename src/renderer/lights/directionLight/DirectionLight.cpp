#include "DirectionLight.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

DirectionLight::DirectionLight(const glm::vec3& dir, const glm::vec3& color) :direction(dir), color(color)
{
	direction = glm::normalize(direction);
	ConstructLightSpaceMatrix(-dir);
}

const glm::vec3& DirectionLight::Direction() const { return direction; }

const glm::vec3& DirectionLight::Color() const { return color; }

const glm::mat4& DirectionLight::LightSpaceMatrix() const { return lightSpaceMatrix; }

void DirectionLight::ConstructLightSpaceMatrix(const glm::vec3& pos)
{
	glm::vec3 eye = pos;
	glm::vec3 center = glm::vec3(0, 0, 0);
	glm::vec3 z = glm::normalize(eye - center);
	glm::vec3 y = glm::vec3(0, 1, 0);
	glm::vec3 x = glm::normalize(glm::cross(y, z));
	y = glm::normalize(glm::cross(z, x));

	glm::mat4 rotation = {
		{ x.x, x.y, x.z, 0 },
		{ y.x, y.y, y.z, 0 },
		{ z.x, z.y, z.z, 0 },
		{ 0,   0,   0,   1 }
	};
	glm::mat4 rotationInvert = {
		{ x.x, y.x, z.x, 0 },
		{ x.y, y.y, z.y, 0 },
		{ x.z, y.z, z.z, 0 },
		{ 0,   0,   0,   1 }
	};

	glm::mat4 translation =
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ eye.x, eye.y, eye.z, 1 },
	};

	glm::mat4 translationInvert =
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ -eye.x, -eye.y, -eye.z, 1 },
	};

	//lookAt  (TR)^-1  =  R^T T^T
	glm::mat4 lookAt = rotationInvert * translationInvert;

	//-------------
	glm::mat4 orthoProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.01f, 10.0f);
	lightSpaceMatrix = orthoProjection * lookAt;
}
#include "DirectionLight.h"

DirectionLight::DirectionLight(const glm::vec3& dir, const glm::vec3& color) :direction(dir), color(color)
{
	direction = glm::normalize(direction);
}

const glm::vec3& DirectionLight::Direction() const { return direction; }

const glm::vec3& DirectionLight::Color() const { return color; }
#include "SpotLight.h"

SpotLight::SpotLight(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& color, float innerCutOff, float outterCutOff):position(position), direction(direction), color(color), innerCutOff(innerCutOff), outterCutOff(outterCutOff)
{
	innerCos  = glm::cos(innerCutOff);
	outterCos = glm::cos(outterCutOff);
	epsilon = innerCos - outterCos;
}

void SpotLight::SetPosition(const glm::vec3& pos)
{
	position = pos;
}

void SpotLight::SetDirection(const glm::vec3& dir)
{
	direction = dir;
}
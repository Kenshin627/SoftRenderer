#include "DirectionLight.h"

DirectionLight::DirectionLight(const glm::vec3& dir, const glm::vec3& color) :direction(glm::normalize(dir)), color(color) 
{
	BuildLightSpace();
}

void DirectionLight::BuildLightSpace()
{
	glm::vec3 center = glm::vec3(0.0f);
	glm::vec3 eye = center - 4.0f * direction;
	glm::vec3 up = glm::vec3(0, 1, 0);
	lightSpace = Camera(eye, center, up);
	lightSpace.OrthoGraphic(-2.0f, 2.0f, -2.0f, 2.0f, 0.01f, 10.0f);
}
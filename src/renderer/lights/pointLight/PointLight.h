#pragma once
#include <glm/glm.hpp>

class PointLight
{
public:
	PointLight() = default;
	PointLight(const glm::vec3& position, const glm::vec3& color, float constant, float linear, float quadratic);
	const glm::vec3& Position() const { return position; }
	const glm::vec3& Color() const { return color; }
private:
	glm::vec3 position;
	glm::vec3 color;	
public:
	float constant;
	float linear;
	float quadratic;
};
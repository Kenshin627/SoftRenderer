#pragma once
#include <glm/glm.hpp>

class DirectionLight
{
public:
	DirectionLight() = default;
	DirectionLight(const glm::vec3& direction, const glm::vec3& color);
	~DirectionLight() = default;
	const glm::vec3& Direction() const;
	const glm::vec3& Color() const;
private:
	glm::vec3 direction { 0, 0, 1 };
	glm::vec3 color { 1, 1, 1 };
};
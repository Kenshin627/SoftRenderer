#pragma once
#include <glm/glm.hpp>

class DirectionLight
{
public:
	DirectionLight() = default;
	DirectionLight(const glm::vec3& direction, const glm::vec3& color);
	~DirectionLight() = default;
	void ConstructLightSpaceMatrix(const glm::vec3& pos);
	const glm::vec3& Direction() const;
	const glm::vec3& Color() const;
	const glm::mat4& LightSpaceMatrix() const;
private:
	glm::vec3 direction { 0, 0, 1 };
	glm::vec3 color { 1, 1, 1 };
	glm::mat4 lightSpaceMatrix;
};
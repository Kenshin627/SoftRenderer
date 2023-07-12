#pragma once
#include <glm/glm.hpp>

class SpotLight
{
public:
	SpotLight() = default;
	SpotLight(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& color, float innerCutOff, float outterCutOff);
	glm::vec3& Position()  { return position; }
	const glm::vec3& Direction() const { return direction; }
	const glm::vec3& Color() const { return color; }
	void SetPosition(const glm::vec3& pos);
	void SetDirection(const glm::vec3& dir);
	float GetEpsilon() const { return epsilon; }
	float GetoutterCos() const { return outterCos; }
	~SpotLight() = default;
private:
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 color;
	float innerCutOff;
	float outterCutOff;
	float innerCos;
	float outterCos;
	float epsilon;
};
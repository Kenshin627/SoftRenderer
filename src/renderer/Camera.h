#pragma once
#include <glm/glm.hpp>

class Camera
{
public:
	Camera() = default;
	Camera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);
	~Camera() = default;
	void Update();
private:
	glm::mat4 view;
	glm::mat4 projection;
};
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	Camera() = default;
	Camera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, float fov, float aspectRatio, float near, float far);
	const glm::mat4& GetView() const { return view; }
	const glm::mat4& GetProjection() const { return projection; }
	const glm::mat4& GetViewProjection() const { return viewProjection; }
	float GetNear() { return zNear; }
	float GetFar() { return zFar; }
	~Camera() = default;
	void Update();
private:
	glm::mat4 view = glm::identity<glm::mat4>();
	glm::mat4 projection = glm::identity<glm::mat4>();
	glm::mat4 viewProjection = glm::identity<glm::mat4>();
	float zNear;
	float zFar;
};
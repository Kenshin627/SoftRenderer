#pragma once
#include "Camera.h"
#include "model.h"
#include "shader/Shader.h"
#include "lights/directionLight/DirectionLight.h"
#include "lights/pointLight/PointLight.h"
#include "lights/spotLight/SpotLight.h"
#include <glm/glm.hpp>
#include <tgaimage/tgaimage.h>
#include <vector>
#include <memory>

class Window;

struct BoundingBox
{
	glm::vec2 min;
	glm::vec2 max;
};

struct FrameBuffer
{
	TGAImage colorAttachment;
	TGAImage depthAttachment;
	float* zBuffer = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	~FrameBuffer()
	{
		delete [] zBuffer;
	}

	void Clear()
	{
		colorAttachment.clear();
		depthAttachment.clear();
		for (uint32_t i = 0; i < width * height; i++)
		{
			zBuffer[i] = std::numeric_limits<double>::max();
		}
	}
};

class Renderer
{
public:
	Renderer() = default;
	Renderer(Window* presentDevice, uint32_t width, uint32_t height);
	~Renderer() = default;
	Renderer(Renderer & r) = default;
	void InitCamera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, float fov, float aspectRatio, float near, float far);
	void InitLight();
	void Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	Camera& GetCamera() { return camera; }
	void Draw(uint64_t deltaTime);
	void rasterize(glm::vec4* clipVertices, glm::vec3* worldCoords);
	BoundingBox GetBoundingBox(glm::vec4* vertices);
	glm::vec3 BaryCentric(glm::vec4* vertices, glm::vec2& p);
	float LinearDepth(float near, float far, float depth);
	void computeTBN(glm::vec3* positions, glm::vec2* uvs, std::unique_ptr<Shader>& shader);
	void Clear();
private:
	Window* presentDevice;
	Camera camera;
	glm::mat4 viewport;
	FrameBuffer frameBuffer;	
	std::vector<Model> models;
	std::unique_ptr<Shader> shader;
	DirectionLight dlight;
	PointLight pLight;
	SpotLight sLight;
};
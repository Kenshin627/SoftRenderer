#pragma once
#include <glm/glm.hpp>
#include "Camera.h"
#include <tgaimage/tgaimage.h>
#include <vector>
#include "model.h"

class Renderer
{
public:
	Renderer(uint32_t width, uint32_t height);
	~Renderer();
	void InitCamera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);
	void Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	void Draw();
	void Clear();
private:
	Camera camera;
	glm::mat4 viewport;
	TGAImage frameBuffer;
	double* zBuffer;
	uint32_t screenwidth;
	uint32_t screenheight;
	uint32_t pixelCount;
	uint32_t halfWidth;
	uint32_t halfHeight;
	std::vector<Model> models;
};
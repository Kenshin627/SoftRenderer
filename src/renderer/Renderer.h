#pragma once
#include <glm/glm.hpp>
#include "Camera.h"
#include <tgaimage/tgaimage.h>
#include <vector>
#include "model.h"
#include <sdl2/include/SDL.h>

struct BoundingBox
{
	glm::vec2 min;
	glm::vec2 max;
};

struct FrameBuffer
{
	TGAImage colorAttachment;
	TGAImage depthAttachment;
	float* zBuffer;
	uint32_t width;
	uint32_t height;
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
	Renderer(SDL_Renderer* presentDevice, uint32_t width, uint32_t height);
	~Renderer() = default;
	void InitCamera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, float fov, float aspectRatio, float near, float far);
	void Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	Camera& GetCamera() { return camera; }
	void Draw();
	void rasterize(glm::vec4* clipVertices, const TGAColor& color);
	BoundingBox GetBoundingBox(glm::vec4* vertices);
	glm::vec3 BaryCentric(glm::vec4* vertices, glm::vec2& p);
	float LinearDepth(float near, float far, float depth);
	void Clear();
private:
	SDL_Renderer* presentDevice;
	Camera camera;
	glm::mat4 viewport;
	FrameBuffer frameBuffer;	
	glm::mat3 sdlCoords;
	std::vector<Model> models;
};
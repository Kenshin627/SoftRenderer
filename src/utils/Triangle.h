#pragma once
#include <tgaimage/tgaimage.h>
#include "glm/glm.hpp"
#include <sdl2/include/SDL.h>

struct BoundingBox
{
	glm::vec2 min;
	glm::vec2 max;
};

void SwapLineTriangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2, TGAImage& image, TGAColor color);

void VertexSortVertical(glm::vec2& v0, glm::vec2& v1, glm::vec2& v2);

void SweepLine(glm::vec2& v0, glm::vec2& v1, glm::vec2& v2, TGAImage& image, TGAColor color);

BoundingBox GetBoundingBox(glm::vec3* vertices, glm::vec2 min, glm::vec2 max);

void BaryCentricTriangle(glm::vec4* clipVertices, TGAImage& image, TGAImage& depthBuffer, const TGAColor& color, float* zBuffer, const glm::mat4& viewport, SDL_Renderer* renderer);
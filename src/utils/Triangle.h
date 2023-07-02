#pragma once
#include <tgaimage/tgaimage.h>
#include "glm/glm.hpp"

struct BoundingBox
{
	glm::vec2 min;
	glm::vec2 max;
};

void SwapLineTriangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2, TGAImage& image, TGAColor color);

void VertexSortVertical(glm::vec2& v0, glm::vec2& v1, glm::vec2& v2);

void SweepLine(glm::vec2& v0, glm::vec2& v1, glm::vec2& v2, TGAImage& image, TGAColor color);

BoundingBox GetBoundingBox(glm::vec2* vertices, glm::vec2 min, glm::vec2 max);

void BaryCentricTriangle(glm::vec2* vertices, TGAImage& image, const TGAColor& color);
#include "Triangle.h"

void SwapLineTriangle(glm::vec2 v0, glm::vec2 v1, glm::vec2 v2, TGAImage& image, TGAColor color)
{
	//1:排序
	VertexSortVertical(v0, v1, v2);

	//2:水平拆分三角形,一分为二,先画下半部分
	SweepLine(v0, v1, v2, image, color);
}

//TODO:对三角形三个顶点按照y值从小到大排序
void VertexSortVertical(glm::vec2& v0, glm::vec2& v1, glm::vec2& v2)
{
	if (v0.y > v1.y)
	{
		std::swap(v0, v1);
	}

	if (v0.y > v2.y)
	{
		std::swap(v0, v2);
	}

	if (v1.y > v2.y)
	{
		std::swap(v1, v2);
	}
}

//TODO:逐行扫描填充像素(扫描线算法)
void SweepLine(glm::vec2& v0, glm::vec2& v1, glm::vec2& v2, TGAImage& image, TGAColor color)
{
	int downSegmentHeight = v1.y - v0.y + 1;
	int upSegmentHeight = v2.y - v1.y + 1;
	int totalHeight = v2.y - v0.y;
	for (uint32_t y = v0.y; y <= v1.y; y++)
	{
		float alpha = (float)(y - v0.y) / totalHeight;
		float beta = (float)(y - v0.y) / downSegmentHeight;
		glm::vec2 sideA = v0 + (v2 - v0) * alpha;
		glm::vec2 sideB = v0 + (v1 - v0) * beta;
		if (sideA.x > sideB.x)
		{
			std::swap(sideA, sideB);
		}
		for (uint32_t x = sideA.x; x <= sideB.x; x++)
		{
			image.set(x, y, color);
		}
	}

	//3:画上半部分
	for (uint32_t y = v1.y; y <= v2.y; y++)
	{
		float alpha = (float)(y - v0.y) / totalHeight;
		float beta = (float)(y - v1.y) / upSegmentHeight;
		glm::vec2 sideA = v0 + (v2 - v0) * alpha;
		glm::vec2 sideB = v1 + (v2 - v1) * beta;
		if (sideA.x > sideB.x)
		{
			std::swap(sideA, sideB);
		}
		for (uint32_t x = sideA.x; x <= sideB.x; x++)
		{
			image.set(x, y, color);
		}
	}
}

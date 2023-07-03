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

glm::vec3 BaryCentric(glm::vec3* vertices, glm::vec3& p)
{	
	glm::vec3 v0 = vertices[0];
	glm::vec3 v1 = vertices[1];
	glm::vec3 v2 = vertices[2];
	glm::vec3 AB = v1 - v0;
	glm::vec3 AC = v2 - v0;
	glm::vec3 PA = v0 -  p;
	glm::vec3 A1 = glm::vec3{ AB.x, AC.x, PA.x };
	glm::vec3 A2 = glm::vec3{ AB.y, AC.y, PA.y };
	glm::vec3 uv1 = cross(A1, A2);
	if (uv1.z < 0)
	{
		return { -1, 1, 1 };
	}

	return { 1.0 - (uv1.x + uv1.y) / uv1.z, uv1.y / uv1.z, uv1.x / uv1.z };
}

BoundingBox GetBoundingBox(glm::vec3* vertices, glm::vec2 min, glm::vec2 max)
{
	BoundingBox bbox;
	bbox.min = max;
	bbox.max = min;
	for (uint32_t i = 0; i < 3; i++)
	{
		bbox.min.x = std::max(min.x, std::min(bbox.min.x, vertices[i].x));
		bbox.min.y = std::max(min.y, std::min(bbox.min.y, vertices[i].y));

		bbox.max.x = std::min(max.x, std::max(bbox.max.x, vertices[i].x));
		bbox.max.y = std::min(max.y, std::max(bbox.max.y, vertices[i].y));
	}
	return bbox;
}

void BaryCentricTriangle(glm::vec3* vertices, TGAImage& image, TGAImage& depthBuffer, const TGAColor& color, double* zBuffer)
{
	int width = image.get_width();
	int height = image.get_height();
	//1: 获取包围盒
	BoundingBox bbox = GetBoundingBox(vertices, { 0, 0 }, { width - 1.0, height - 1.0 });

	glm::vec3 p;
	//2: 遍历包围盒获取当前像素点的重心坐标
	for (p.x = bbox.min.x; p.x<= bbox.max.x; p.x++)
	{
		for (p.y = bbox.min.y; p.y<= bbox.max.y; p.y++) 
		{
			p.z = 0;
			glm::vec3 bc = BaryCentric(vertices, p);
			//3:判断是否在三角形内,如果在三角形内,计算像素颜色
			if (bc.x < 0 || bc.y < 0 || bc.z < 0)
			{
				continue;
			}
			//4:判断zbuffer
			for (uint32_t i = 0; i < 3; i++)
			{
				p.z += vertices[i].z * bc[i];
			}
			int bufferIndex = (int)(p.x + p.y * width);
			if (p.z > zBuffer[bufferIndex])
			{
				zBuffer[bufferIndex] = p.z;
				image.set(p.x, p.y, color);
				depthBuffer.set(p.x, p.y, TGAColor(p.z, p.z, p.z, 255));
			}
		}
	}
}
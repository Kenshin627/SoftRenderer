#include "Triangle.h"

void Triangle(vec2 v0, vec2 v1, vec2 v2, TGAImage& image, TGAColor color)
{
	//1:����
	VertexSortVertical(v0, v1, v2);

	//2:ˮƽ���������,һ��Ϊ��,�Ȼ��°벿��
	SweepLine(v0, v1, v2, image, color);
}

//TODO:���������������㰴��yֵ��С��������
void VertexSortVertical(vec2& v0, vec2& v1, vec2& v2)
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

//TODO:����ɨ���������(ɨ�����㷨)
void SweepLine(vec2& v0, vec2& v1, vec2& v2, TGAImage& image, TGAColor color)
{
	int downSegmentHeight = v1.y - v0.y + 1;
	int upSegmentHeight = v2.y - v1.y + 1;
	int totalHeight = v2.y - v0.y;
	for (uint32_t y = v0.y; y <= v1.y; y++)
	{
		float alpha = (float)(y - v0.y) / totalHeight;
		float beta = (float)(y - v0.y) / downSegmentHeight;
		vec2 sideA = v0 + (v2 - v0) * alpha;
		vec2 sideB = v0 + (v1 - v0) * beta;
		if (sideA.x > sideB.x)
		{
			std::swap(sideA, sideB);
		}
		for (uint32_t x = sideA.x; x <= sideB.x; x++)
		{
			image.set(x, y, color);
		}
	}

	//3:���ϰ벿��
	for (uint32_t y = v1.y; y <= v2.y; y++)
	{
		float alpha = (float)(y - v0.y) / totalHeight;
		float beta = (float)(y - v1.y) / upSegmentHeight;
		vec2 sideA = v0 + (v2 - v0) * alpha;
		vec2 sideB = v1 + (v2 - v1) * beta;
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
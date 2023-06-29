#include "Line.h"

//使用线性插值函数，高度依赖t的增量，如果t取值大，渲染的像素点少，不足以形成直线，会变成虚线，如果取值小，造成性能浪费
void DrawLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, TGAImage& image, TGAColor color)
{
	for (float t = 0; t < 1; t += 0.01)
	{
		int x = x0 * (1 - t) + x1 * t;
		int y = y0 * (1 - t) + y1 * t;
		image.set(x, y, color);
	}
}

//根据二维平面某一个轴的坐标范围来递增：比如ｘ轴，但是有个问题，当线段的斜率大于１或者小于－１，ｘ的步进无法跟上ｙ的快速变化，绘制的直线会变成虚线
void DrawLine2(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, TGAImage& image, TGAColor color)
{
	if (x0 > x1)
	{
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	uint32_t xRange = x1 - x0;
	for (uint32_t x = x0; x <= x1; x++)
	{
		float t = (float)(x - x0) / xRange;
		uint32_t y = y0 * (1.0 - t) + t * y1;
		image.set(x, y, color);
	}
}

void DrawLine3(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, TGAImage& image, TGAColor color)
{
	float dx = abs(static_cast<int>(x1 - x0));
	float dy = abs(static_cast<int>(y1 - y0));
	//记录当前线段是否斜率大于1或小于-1，如果是，则需要用y值做递增，在实际绘制像素点时反转x，y
	float steep = false;
	if (dx < dy)
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}
	
	if (x0 > x1)
	{
	
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	uint32_t xRange = x1 - x0;
	for (uint32_t x = x0; x <= x1; x++) 
	{
		float t = (float)(x - x0) / xRange;
		uint32_t y = y0 * (1 - t) + y1 * t;
		if (steep)
		{
			image.set(y, x, color);
		}
		else
		{
			image.set(x, y, color);
		}
	}
}

void DrawLine4(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, TGAImage& image, TGAColor color)
{
	bool steep = false;
	if (abs(static_cast<int>(x1 - x0)) < abs(static_cast<int>(y1 - y0)))
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}

	if (x0 > x1)
	{
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int dx = x1 - x0;
	int dy = y1 - y0;
	int derror2 = std::abs(dy) * 2;
	int error2 = 0;
	int y = y0;
	for (uint32_t x = x0; x <= x1 ; x++)
	{
		if (steep)
		{
			image.set(y, x, color);
		}
		else
		{
			image.set(x, y, color);
		}
		error2 += derror2;
		if (error2 > dx)
		{
			y += (y1 > y0 ? 1 : -1);
			error2 -= dx * 2;
		}
	}
}

#include "FrameBuffer.h"

FrameBuffer::FrameBuffer(uint32_t width, uint32_t height, bool depthOnly):width(width), height(height)
{
	if (!depthOnly)
	{
		colorAttachment = TGAImage(width, height, TGAImage::RGB);
		normalAttachment = TGAImage(width, height, TGAImage::RGB);
	}
	zBuffer = new float[width * height];
	Reset();
}

FrameBuffer::~FrameBuffer()
{
	delete[] zBuffer;
}

void FrameBuffer::Reset()
{
	if (colorAttachment.get_width()) colorAttachment.clear();
	if (clearColor.val != 0)
	{
		for (uint32_t y = 0; y < height; y++)
		{
			for (uint32_t x = 0; x < width; x++)
			{
				colorAttachment.set((int)x, (int)y, clearColor);
			}
		}
	}
	for (uint32_t i = 0; i < width * height; i++)
	{
		zBuffer[i] = std::numeric_limits<float>::max();
	}
}
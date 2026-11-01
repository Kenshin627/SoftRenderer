#pragma once
#include <tgaimage/tgaimage.h>

struct FrameBuffer
{
	FrameBuffer(uint32_t width, uint32_t height);
	~FrameBuffer();
	void Reset();
	TGAImage colorAttachment;
	float* zBuffer = nullptr;
	uint32_t width;
	uint32_t height;
};
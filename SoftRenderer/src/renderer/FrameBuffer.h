#pragma once
#include <tgaimage/tgaimage.h>

struct FrameBuffer
{
	FrameBuffer(uint32_t width, uint32_t height);
	~FrameBuffer();
	void Reset();
	TGAImage colorAttachment;
	TGAImage normalAttachment; // world-space normal * 0.5 + 0.5, for AO passes
	float* zBuffer = nullptr;
	uint32_t width;
	uint32_t height;
};
#pragma once
#include <tgaimage/tgaimage.h>
#include <array>
#include <vector>

struct FrameBuffer
{
	FrameBuffer(uint32_t width, uint32_t height, bool depthOnly = false);
	~FrameBuffer();
	void Reset();
	struct Sample
	{
		float depth;
		uint8_t r, g, b;
	};
	using SampleQuad = std::array<Sample, 4>;
	std::vector<SampleQuad> samples;
	void FinishColor(bool visualizeDepth);
	TGAImage colorAttachment;
	TGAImage normalAttachment; // world-space normal * 0.5 + 0.5, for AO passes
	float* zBuffer = nullptr;
	uint32_t width;
	uint32_t height;
	TGAColor clearColor = { 0, 0, 0, 0 }; // background fill for the color attachment
};
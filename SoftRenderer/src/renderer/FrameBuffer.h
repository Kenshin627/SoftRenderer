#pragma once
#include <tgaimage/tgaimage.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

struct HdrSettings
{
	bool enabled = false;
	float exposureEv = 0.0f;
	float bloomStrength = 0.0f;
	float emissionStrength = 1.0f;
};

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
	// One linear RGB triplet per coverage sample, allocated only for HDR camera passes.
	std::vector<glm::vec3> linearColor;
	void WriteFragment(int x, int y, size_t pixel, int visibleMask, const float depths[4], const glm::vec3& rgb);
	void FinishHdr(const HdrSettings& settings);
	void FinishColor(bool visualizeDepth);
	TGAImage colorAttachment;
	TGAImage normalAttachment; // world-space normal * 0.5 + 0.5, for AO passes
	float* zBuffer = nullptr;
	uint32_t width;
	uint32_t height;
	TGAColor clearColor = { 0, 0, 0, 0 }; // background fill for the color attachment
};
#include "FrameBuffer.h"
#include "../texture/ColorSpace.h"
#include <algorithm>
#include <limits>

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
	if (!samples.empty())
	{
		SampleQuad background;
		background.fill({std::numeric_limits<float>::max(), clearColor.r, clearColor.g, clearColor.b});
		std::fill(samples.begin(), samples.end(), background);
	}
	if (!linearColor.empty())
	{
		glm::vec3 clear(ColorSpace::DecodeSrgb(clearColor.r), ColorSpace::DecodeSrgb(clearColor.g),
		                ColorSpace::DecodeSrgb(clearColor.b));
		std::fill(linearColor.begin(), linearColor.end(), clear);
	}
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

void FrameBuffer::WriteFragment(int x, int y, size_t pixel, int visibleMask, const float depths[4], const glm::vec3& rgb)
{
	const bool hdr = !linearColor.empty();
	TGAColor encoded;
	if (!hdr)
	{
		glm::vec3 bytes = glm::clamp(rgb, 0.0f, 1.0f) * 255.0f;
		encoded = TGAColor(uint8_t(bytes.r), uint8_t(bytes.g), uint8_t(bytes.b), 255);
	}
	if (samples.empty())
	{
		if (hdr) linearColor[pixel] = rgb;
		else colorAttachment.set(x, y, encoded);
		return;
	}
	auto& quad = samples[pixel];
	for (int i = 0; i < 4; ++i)
	{
		if (!(visibleMask & (1 << i))) continue;
		if (hdr)
		{
			quad[i].depth = depths[i];
			linearColor[4 * pixel + i] = rgb;
		}
		else quad[i] = {depths[i], encoded.r, encoded.g, encoded.b};
	}
}

void FrameBuffer::BlendFragment(size_t pixel, int visibleMask, const float alpha[4], const glm::vec3& rgb)
{
	const int count = samples.empty() ? 1 : 4;
	for (int i = 0; i < count; ++i)
	{
		if (!(visibleMask & (1 << i))) continue;
		auto& destination = linearColor[pixel * count + i];
		destination = glm::mix(destination, rgb, alpha[i]);
	}
}

void FrameBuffer::FinishColor(bool visualizeDepth)
{
	if (samples.empty() && !visualizeDepth) return;
	const float empty = std::numeric_limits<float>::max();
	float nearDepth = 1.0f, farDepth = -1.0f;
	if (visualizeDepth)
	{
		auto include = [&](float depth) {
			if (depth == empty) return;
			nearDepth = std::min(nearDepth, depth);
			farDepth = std::max(farDepth, depth);
		};
		if (samples.empty())
			for (size_t i = 0; i < size_t(width) * height; ++i) include(zBuffer[i]);
		else
			for (const auto& quad : samples)
				for (const auto& s : quad) include(s.depth);
	}
	auto gray = [&](float depth) {
		return uint8_t(255.0f * (depth - nearDepth) / std::max(farDepth - nearDepth, 1e-6f));
	};
	for (uint32_t y = 0; y < height; ++y)
		for (uint32_t x = 0; x < width; ++x)
		{
			size_t i = x + size_t(y) * width;
			if (samples.empty())
			{
				if (zBuffer[i] == empty) continue;
				auto v = gray(zBuffer[i]);
				colorAttachment.set(x, y, TGAColor(v, v, v, 255));
				continue;
			}
			int r = 0, g = 0, b = 0;
			for (const Sample& s : samples[i])
			{
				if (visualizeDepth && s.depth != empty)
				{
					auto v = gray(s.depth);
					r += v; g += v; b += v;
				}
				else { r += s.r; g += s.g; b += s.b; }
			}
			colorAttachment.set(x, y, TGAColor((r + 2) / 4, (g + 2) / 4, (b + 2) / 4, 255));
		}
}

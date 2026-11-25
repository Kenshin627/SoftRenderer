#include "FrameBuffer.h"
#include "../texture/ColorSpace.h"
#include <cmath>

namespace
{
	struct BloomImage
	{
		int width, height;
		std::vector<glm::vec3> pixels;

		BloomImage(int w, int h) : width(w), height(h), pixels(size_t(w) * h) {}

		const glm::vec3& At(int x, int y) const
		{
			return pixels[size_t(glm::clamp(y, 0, height - 1)) * width + glm::clamp(x, 0, width - 1)];
		}

		glm::vec3 Upsample(int x, int y) const
		{
			// Each half-resolution texel is centered between its four contributing pixels.
			float u = x * 0.5f - 0.25f, v = y * 0.5f - 0.25f;
			int left = int(std::floor(u)), bottom = int(std::floor(v));
			return glm::mix(glm::mix(At(left, bottom), At(left + 1, bottom), u - left),
			                glm::mix(At(left, bottom + 1), At(left + 1, bottom + 1), u - left), v - bottom);
		}
	};

	BloomImage BuildBloom(const std::vector<glm::vec3>& scene, int width, int height)
	{
		BloomImage bright((width + 1) / 2, (height + 1) / 2);
		for (int y = 0; y < bright.height; ++y)
			for (int x = 0; x < bright.width; ++x)
			{
				glm::vec3 sum(0.0f);
				for (int dy = 0; dy < 2; ++dy)
					for (int dx = 0; dx < 2; ++dx)
					{
						int sx = glm::min(2 * x + dx, width - 1), sy = glm::min(2 * y + dy, height - 1);
						const auto& rgb = scene[size_t(sy) * width + sx];
						float luminance = glm::dot(rgb, glm::vec3(0.2126f, 0.7152f, 0.0722f));
						if (luminance > 1.0f) sum += rgb * ((luminance - 1.0f) / luminance);
					}
				bright.pixels[size_t(y) * bright.width + x] = 0.25f * sum;
			}

		// A fixed fraction of the image height/width keeps the apparent radius stable when resized.
		float sigma = glm::max(1.0f, 0.004f * glm::min(width, height));
		int radius = int(std::ceil(3.0f * sigma));
		std::vector<float> weights(2 * radius + 1);
		float total = 0.0f;
		for (int i = -radius; i <= radius; ++i)
		{
			float weight = std::exp(-float(i * i) / (2.0f * sigma * sigma));
			weights[i + radius] = weight;
			total += weight;
		}
		for (auto& weight : weights) weight /= total;

		BloomImage scratch(bright.width, bright.height);
		for (int axis = 0; axis < 2; ++axis)
		{
			for (int y = 0; y < bright.height; ++y)
				for (int x = 0; x < bright.width; ++x)
				{
					glm::vec3 sum(0.0f);
					for (int i = -radius; i <= radius; ++i)
						sum += bright.At(x + (axis == 0 ? i : 0), y + (axis == 1 ? i : 0)) * weights[i + radius];
					scratch.pixels[size_t(y) * bright.width + x] = sum;
				}
			bright.pixels.swap(scratch.pixels);
		}
		return bright;
	}

	uint8_t DisplayByte(float radiance, float exposure)
	{
		float exposed = glm::max(0.0f, radiance) * exposure;
		float mapped = exposed / (1.0f + exposed);
		return uint8_t(std::lround(255.0f * ColorSpace::EncodeSrgb(mapped)));
	}
}

void FrameBuffer::FinishHdr(const HdrSettings& settings)
{
	if (!samples.empty())
	{
		// Compact resolved pixels in place, reading all four samples before overwriting.
		for (size_t i = 0; i < samples.size(); ++i)
		{
			const auto* colors = &linearColor[4 * i];
			linearColor[i] = 0.25f * (colors[0] + colors[1] + colors[2] + colors[3]);
		}
	}
	const float exposure = std::exp2(settings.exposureEv);
	const BloomImage bloom = settings.bloomStrength > 0.0f ? BuildBloom(linearColor, width, height) : BloomImage(0, 0);
	for (uint32_t y = 0; y < height; ++y)
		for (uint32_t x = 0; x < width; ++x)
		{
			glm::vec3 rgb = linearColor[size_t(y) * width + x];
			if (!bloom.pixels.empty()) rgb += settings.bloomStrength * bloom.Upsample(x, y);
			colorAttachment.set(x, y, TGAColor(DisplayByte(rgb.r, exposure), DisplayByte(rgb.g, exposure),
			                                   DisplayByte(rgb.b, exposure), 255));
		}
}

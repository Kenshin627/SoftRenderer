#include "Texture.h"
#include "ColorSpace.h"
#include <stbimage/stb_image.h>
#include <cmath>
#include <memory>

Texture::Texture(const char* path)
{
	int width = 0, height = 0, channels = 0;
	std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
		stbi_load(path, &width, &height, &channels, STBI_rgb_alpha), stbi_image_free);
	if (!pixels) return;
	hasAlpha = channels == 2 || channels == 4;
	base.width = width;
	base.height = height;
	base.rgba.assign(pixels.get(), pixels.get() + size_t(width) * height * 4);
}

glm::vec4 Texture::Level::Texel(int x, int y, bool srgb) const
{
	const auto* p = &rgba[4 * (size_t(y) * width + x)];
	if (srgb)
		return {255.0f * ColorSpace::DecodeSrgb(p[0]), 255.0f * ColorSpace::DecodeSrgb(p[1]),
		        255.0f * ColorSpace::DecodeSrgb(p[2]), p[3]};
	return {p[0], p[1], p[2], p[3]};
}

glm::vec4 Texture::Get(uint32_t x, uint32_t y) const
{
	if (base.rgba.empty()) return glm::vec4(0);
	return base.Texel(x % base.width, y % base.height, false);
}

glm::vec4 Texture::Level::Bilinear(const glm::vec2& uv, bool srgb) const
{
	// Texel centers lie at (i + 0.5) / size. Wrap each tap across the tile seam.
	const glm::vec2 pos = glm::fract(uv) * glm::vec2(width, height) - 0.5f;
	const glm::ivec2 lo(glm::floor(pos));
	const glm::vec2 f = glm::fract(pos);
	const int x0 = (lo.x + width) % width, x1 = (lo.x + 1) % width;
	const int y0 = (lo.y + height) % height, y1 = (lo.y + 1) % height;
	return glm::mix(glm::mix(Texel(x0, y0, srgb), Texel(x1, y0, srgb), f.x),
		glm::mix(Texel(x0, y1, srgb), Texel(x1, y1, srgb), f.x), f.y);
}

Texture::Level Texture::Level::Downsample(bool srgb) const
{
	Level dst;
	dst.width = glm::max(1, width / 2);
	dst.height = glm::max(1, height / 2);
	dst.rgba.resize(size_t(dst.width) * dst.height * 4);
	// Area weights include the last row/column even when a dimension is odd.
	for (int y = 0; y < dst.height; ++y)
	{
		const double top = double(y) * height / dst.height;
		const double bottom = double(y + 1) * height / dst.height;
		for (int x = 0; x < dst.width; ++x)
		{
			const double left = double(x) * width / dst.width;
			const double right = double(x + 1) * width / dst.width;
			glm::dvec4 sum(0.0);
			for (int sy = int(top); sy < bottom; ++sy)
			{
				const double wy = glm::min(bottom, double(sy + 1)) - glm::max(top, double(sy));
				for (int sx = int(left); sx < right; ++sx)
				{
					const double wx = glm::min(right, double(sx + 1)) - glm::max(left, double(sx));
					sum += glm::dvec4(Texel(sx, sy, srgb)) * (wx * wy);
				}
			}
			const auto mean = sum / ((right - left) * (bottom - top));
			for (int c = 0; c < 4; ++c)
			{
				double value = mean[c];
				if (srgb && c < 3) value = 255.0 * ColorSpace::EncodeSrgb(float(value / 255.0));
				dst.rgba[(size_t(y) * dst.width + x) * 4 + c] = uint8_t(std::lround(value));
			}
		}
	}
	return dst;
}

float Texture::OpacityChannel(glm::vec4 rgba, char channel)
{
	switch (channel)
	{
	case 'r': return rgba.r;
	case 'g': return rgba.g;
	case 'b': return rgba.b;
	case 'm': return rgba.a;
	default:
		if (rgba.r == rgba.g && rgba.g == rgba.b) return rgba.r;
		return glm::dot(glm::vec3(rgba), glm::vec3(0.2126f, 0.7152f, 0.0722f));
	}
}

AlphaMode Texture::ClassifyOpacity(char channel, float dissolve) const
{
	bool holes = false;
	for (size_t i = 0; i < base.rgba.size(); i += 4)
	{
		const auto* p = base.rgba.data() + i;
		float value = dissolve * OpacityChannel(glm::vec4(p[0], p[1], p[2], p[3]) / 255.0f, channel);
		if (value > 0.0f && value < 1.0f) return AlphaMode::Blend;
		holes |= value == 0.0f;
	}
	return holes ? AlphaMode::Mask : AlphaMode::Opaque;
}

namespace {
struct FilterFootprint
{
	float widthSquared;
	int taps = 1;
	glm::vec2 stepUv{0.0f};
};

FilterFootprint AnisotropicFootprint(glm::dvec2 dx, glm::dvec2 dy, glm::vec2 size, int limit)
{
	// J maps a screen-space step to texels. The eigenvectors of J J^T are the
	// texture-space axes, even when the projected UVs are rotated or sheared.
	const double uu = dx.x * dx.x + dy.x * dy.x;
	const double vv = dx.y * dx.y + dy.y * dy.y;
	const double uv = dx.x * dx.y + dy.x * dy.y;
	const double gap = std::sqrt((uu - vv) * (uu - vv) + 4.0 * uv * uv);
	const double majorSquared = (uu + vv + gap) * 0.5;
	if (majorSquared <= 1.0) return {1.0f};

	const double major = std::sqrt(majorSquared);
	// The product of the axis lengths is |det J|, also valid for a line footprint.
	const double minor = std::abs(dx.x * dy.y - dx.y * dy.x) / major;
	const double width = glm::max(1.0, glm::max(minor, major / limit));
	const int taps = int(std::ceil(glm::min(double(limit), major / width)));
	if (taps <= 1 || gap == 0.0) return {float(majorSquared)};

	const glm::dvec2 axis = uu >= vv ? glm::dvec2(majorSquared - vv, uv) : glm::dvec2(uv, majorSquared - uu);
	// Leave half a tap width at both ends so the taps cover the footprint.
	const glm::dvec2 step = glm::normalize(axis) * ((major - width) / (taps - 1));
	return {float(width * width), taps, glm::vec2(step / glm::dvec2(size))};
}
}

glm::vec3 Texture::Sample(const glm::vec2& uv, TextureFilter filter,
	const glm::vec2& uvDx, const glm::vec2& uvDy, bool srgb, int maxAnisotropy) const
{
	return glm::vec3(SampleRgba(uv, filter, uvDx, uvDy, srgb, maxAnisotropy));
}

glm::vec4 Texture::SampleRgba(const glm::vec2& uv, TextureFilter filter,
	const glm::vec2& uvDx, const glm::vec2& uvDy, bool srgb, int maxAnisotropy) const
{
	if (base.rgba.empty()) return glm::vec4(0);
	if (filter == TextureFilter::Nearest)
	{
		// Preserve the original nearest sampler's endpoint convention.
		auto texel = [](float u, int size) {
			float wrapped = u == 1.0f ? 1.0f : glm::fract(u);
			return int(glm::min(wrapped * size, float(size - 1)));
		};
		return base.Texel(texel(uv.x, base.width), texel(uv.y, base.height), srgb) / 255.0f;
	}
	if (filter == TextureFilter::Bilinear) return base.Bilinear(uv, srgb) / 255.0f;

	const glm::vec2 size(base.width, base.height);
	const auto dx = uvDx * size, dy = uvDy * size;
	FilterFootprint footprint{glm::max(glm::dot(dx, dx), glm::dot(dy, dy))};
	if (filter == TextureFilter::Trilinear && maxAnisotropy > 1)
		footprint = AnisotropicFootprint(dx, dy, size, glm::min(maxAnisotropy, 8));
	const Level* first = &base;
	const Level* second = &base;
	float blend = 0.0f;
	if (footprint.widthSquared > 1.0f)
	{
		auto& mips = srgb ? colorMips : rawMips;
		std::call_once(srgb ? colorMipInit : rawMipInit, [this, srgb, &mips] {
			std::vector<Level> chain;
			const Level* level = &base;
			while (level->width > 1 || level->height > 1)
			{
				chain.push_back(level->Downsample(srgb));
				level = &chain.back();
			}
			mips.swap(chain);
		});
		const float lod = glm::min(0.5f * std::log2(footprint.widthSquared), float(mips.size()));
		auto level = [&](int i) -> const Level& { return i == 0 ? base : mips[i - 1]; };
		if (filter == TextureFilter::Mipmap)
			return level(int(std::round(lod))).Bilinear(uv, srgb) / 255.0f;
		const int lower = int(lod);
		first = &level(lower);
		if (lower == int(mips.size())) return first->Bilinear(uv, srgb) / 255.0f;
		blend = lod - lower;
		second = &level(lower + 1);
	}
	auto trilinear = [&](const glm::vec2& at) {
		const auto a = first->Bilinear(at, srgb);
		if (blend == 0.0f) return a;
		return glm::mix(a, second->Bilinear(at, srgb), blend);
	};
	if (footprint.taps == 1) return trilinear(uv) / 255.0f;
	glm::vec4 sum(0.0f);
	for (int i = 0; i < footprint.taps; ++i)
	{
		const float offset = i - 0.5f * (footprint.taps - 1);
		sum += trilinear(uv + offset * footprint.stepUv);
	}
	return sum / (255.0f * footprint.taps);
}

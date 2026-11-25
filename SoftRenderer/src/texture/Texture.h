#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <mutex>
#include <vector>

enum class AlphaMode { Auto, Opaque, Mask, Blend };

enum class TextureFilter { Nearest, Bilinear, Mipmap, Trilinear };

class Texture
{
public:
	Texture() = default;
	explicit Texture(const char* path);
	int GetWidth() const { return base.width; }
	int GetHeight() const { return base.height; }
	glm::vec4 Get(uint32_t x, uint32_t y) const;
	bool HasAlpha() const { return hasAlpha; }
	AlphaMode ClassifyOpacity(char channel, float dissolve) const;
	glm::vec4 SampleRgba(const glm::vec2& uv, TextureFilter filter,
		const glm::vec2& uvDx, const glm::vec2& uvDy, bool srgb = false, int maxAnisotropy = 1) const;
	static float OpacityChannel(glm::vec4 rgba, char channel);
	glm::vec3 Sample(const glm::vec2& uv, TextureFilter filter,
		const glm::vec2& uvDx, const glm::vec2& uvDy, bool srgb = false, int maxAnisotropy = 1) const;
private:
	struct Level
	{
		int width = 0, height = 0;
		std::vector<uint8_t> rgba;
		glm::vec4 Texel(int x, int y, bool srgb) const;
		glm::vec4 Bilinear(const glm::vec2& uv, bool srgb) const;
		Level Downsample(bool srgb) const;
	};

	bool hasAlpha = false;
	Level base;
	mutable std::once_flag rawMipInit, colorMipInit;
	mutable std::vector<Level> rawMips, colorMips; // level 1 through the final 1x1 level
};

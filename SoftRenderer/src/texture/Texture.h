#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <mutex>
#include <vector>

enum class TextureFilter { Nearest, Bilinear, Mipmap, Trilinear };

class Texture
{
public:
	Texture() = default;
	explicit Texture(const char* path);
	int GetWidth() const { return base.width; }
	int GetHeight() const { return base.height; }
	glm::vec4 Get(uint32_t x, uint32_t y) const;
	glm::vec3 Sample(const glm::vec2& uv, TextureFilter filter,
		const glm::vec2& uvDx, const glm::vec2& uvDy) const;
private:
	struct Level
	{
		int width = 0, height = 0;
		std::vector<uint8_t> rgba;
		glm::vec4 Texel(int x, int y) const;
		glm::vec4 Bilinear(const glm::vec2& uv) const;
		Level Downsample() const;
	};

	Level base;
	mutable std::once_flag mipInit;
	mutable std::vector<Level> mips; // level 1 through the final 1x1 level
};

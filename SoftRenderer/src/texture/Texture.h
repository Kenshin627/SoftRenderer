#pragma once
#include <glm/glm.hpp>
#include <stbimage/stb_image.h>
#include <stdint.h>

class Texture
{
public:
	Texture() = default;
	Texture(const char* path);
	int GetWidth() const { return width; }
	int GetHeight() const { return height; }
	~Texture();
	glm::vec4 Get(uint32_t w, uint32_t h) const;
private:
	unsigned char* data = nullptr;
	int width = 0;
	int height = 0;
};
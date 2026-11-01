#include "Texture.h"

Texture::Texture(const char* path)
{
	//stbi_set_flip_vertically_on_load(true);
	data = stbi_load(path, &width, &height, &bytePerPixel, 0);
}

Texture::~Texture()
{
	stbi_image_free(data);
}

 glm::vec4 Texture::Get(uint32_t x, uint32_t y) const 
{
	if (data == nullptr)
	{
		return glm::vec4(0.0f);
	}
 unsigned char* pixelOffset = data + (x % width + y % height * width) * bytePerPixel; // wrap for tiled UVs
	unsigned char r = pixelOffset[0];
	unsigned char g = pixelOffset[1];
	unsigned char b = pixelOffset[2];
	unsigned char a = bytePerPixel == 4 ? pixelOffset[3] : 0xff;
	return { r, g, b, a };
}
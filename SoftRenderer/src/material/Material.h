#pragma once
#include "../texture/Texture.h"
#include <memory>

class Material
{
public:
	Material() = default;
public:
	std::shared_ptr<Texture> ao;
	std::shared_ptr<Texture> diffuse;
	std::shared_ptr<Texture> specular;
	std::shared_ptr<Texture> normal;
	std::shared_ptr<Texture> roughness;
	std::shared_ptr<Texture> metalness;
	std::shared_ptr<Texture> emissive; // _glow.tga, self-illumination unaffected by lighting
};
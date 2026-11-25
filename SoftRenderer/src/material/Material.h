#pragma once
#include "../texture/Texture.h"
#include <memory>

class Material
{
public:
	Material() = default;
	void ConfigureOpacity(float dissolve, const std::shared_ptr<Texture>& map, char channel, AlphaMode mode);
	AlphaMode Mode(AlphaMode overrideMode) const;
	float SampleOpacity(const glm::vec2& uv, TextureFilter filter, const glm::vec2& dx, const glm::vec2& dy, int maxAnisotropy = 1) const;

	std::shared_ptr<Texture> ao;
	std::shared_ptr<Texture> diffuse;
	std::shared_ptr<Texture> specular;
	std::shared_ptr<Texture> normal; // tangent space
	std::shared_ptr<Texture> objectNormal;
	std::shared_ptr<Texture> roughness;
	std::shared_ptr<Texture> metalness;
	std::shared_ptr<Texture> emissive; // _glow.tga, self-illumination unaffected by lighting
private:
	float opacity = 1.0f;
	std::shared_ptr<Texture> opacityMap;
	char opacityChannel = 'm';
	AlphaMode alphaMode = AlphaMode::Opaque;
};
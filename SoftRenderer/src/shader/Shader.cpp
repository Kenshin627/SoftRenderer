#include "Shader.h"

void Shader::SetSampler(uint32_t slot, const std::shared_ptr<Texture>& texture)
{
	switch (slot)
	{
	case 0:
		diffuseTexture = texture; break;
	case 1:
		specularTexture = texture; break;
	case 2:
		tbnNormalTexture = texture; break;
	case 3:
		aoTexture = texture; break;
	case 4:
		roughnessTexture = texture; break;
	case 5:
		metalnessTexture = texture; break;
	case 6:
		emissiveTexture = texture; break;
	default:
		break;
	}
}

glm::vec3 Shader::Sampler2D(const glm::vec2& uv, const std::shared_ptr<Texture>& texture)
{
	if (!texture)
	{
		return { 1.0f, 1.0f, 1.0f }; // unbound sampler slot falls back to white
	}
	glm::vec4 color = texture->Get((uint32_t)(uv.x * (texture->GetWidth() - 1.0f)), (uint32_t)(uv.y * (texture->GetHeight() - 1.0f)));
	return glm::vec3(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f);
}
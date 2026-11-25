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
	return texture->Sample(uv, textureFilter, uvDx, uvDy, false, maxAnisotropy);
}

glm::vec3 Shader::SampleColor(const glm::vec2& uv, const std::shared_ptr<Texture>& texture) const
{
	return texture ? texture->Sample(uv, textureFilter, uvDx, uvDy, hdr, maxAnisotropy) : glm::vec3(1.0f);
}

glm::vec3 Shader::SampleEmission(const glm::vec2& uv) const
{
	return emissiveTexture ? emissionStrength * SampleColor(uv, emissiveTexture) : glm::vec3(0.0f);
}

glm::vec3 Shader::SurfaceNormal(const glm::vec2& uv)
{
	const auto& v = triangle.vertices;
	const auto& w = baryCentric;
	glm::vec3 n = glm::normalize(w.x * v[0].normal + w.y * v[1].normal + w.z * v[2].normal);
	if (normalSource == NormalSource::Vertex) return n;
	if (normalSource == NormalSource::Object || (normalSource == NormalSource::Auto && !tbnNormalTexture))
	{
		if (!objectNormalTexture) return n;
		glm::vec3 local = 2.0f * Sampler2D(uv, objectNormalTexture) - 1.0f;
		glm::vec3 mapped = itModel * local;
		return glm::dot(mapped, mapped) > 1e-8f ? glm::normalize(mapped) : n;
	}
	if (!tbnNormalTexture) return n;
	glm::vec3 t = w.x * v[0].tangent + w.y * v[1].tangent + w.z * v[2].tangent;
	glm::vec3 b = w.x * v[0].bitangent + w.y * v[1].bitangent + w.z * v[2].bitangent;
	if (glm::dot(t, t) < 1e-12f || glm::dot(b, b) < 1e-12f) return n;
	glm::vec3 local = 2.0f * Sampler2D(uv, tbnNormalTexture) - 1.0f;
	glm::vec3 mapped = glm::mat3(glm::normalize(t), glm::normalize(b), n) * local;
	// Opposing normals can cancel when neighboring texels are filtered.
	return glm::dot(mapped, mapped) > 1e-8f ? glm::normalize(mapped) : n;
}

#pragma once
#include <glm/glm.hpp>
#include <tgaimage/tgaimage.h>

struct VertexAttribute
{
	glm::vec4 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

class Shader
{
public:
	glm::vec3 baryCentricCoords;
	glm::vec3 baseColor { 0, 0, 0 };
	glm::mat4 modelViewprojection;
	glm::vec3 lightDir { 0, 1, 1 };
	TGAImage diffuseTexture;
	TGAImage specularTexture;
	TGAImage normalMap;
	virtual void Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex) = 0;
	virtual bool Fragment(glm::vec4& gl_FragColor, float intensity) = 0;
	/*
	 * diffuse   - 0
	 * specular  - 1 
	 * normalMap - 2
	 */
	void SetUniformSampler(uint32_t slot, const TGAImage& texture)
	{
		switch (slot)
		{
		case 0:
			diffuseTexture = texture; break;
		case 1:
			specularTexture = texture; break;
		case 2:
			normalMap = texture; break;
		default:
			break;
		}
	}
};


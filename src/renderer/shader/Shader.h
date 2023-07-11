#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tgaimage/tgaimage.h>
#include "../lights/directionLight/DirectionLight.h"

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
	glm::mat4 viewprojectionMatrix;
	glm::mat4 modelMatrix = glm::identity<glm::mat4>();
	glm::mat3 invertTransposeModelMatrix = glm::identity<glm::mat4>();
	DirectionLight dLight;
	glm::vec3 cameraPos;
	TGAImage diffuseTexture;
	TGAImage specularTexture;
	TGAImage normalMap;
	glm::vec3 tangent;
	glm::vec3 bitangent;
	float flatShadeIntensity;
	virtual void Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex) = 0;
	virtual bool Fragment(glm::vec4& gl_FragColor) = 0;
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

	//[0 - 1]
	glm::vec3 Sampler2D(const glm::vec2& uv, TGAImage& image)
	{
		TGAColor c = image.get(uv.x * image.get_width(), uv.y * image.get_height());
		return glm::vec3(c.r / 255.0, c.g / 255.0, c.b / 255.0);
	}
};


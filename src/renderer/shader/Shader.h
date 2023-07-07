#pragma once
#include <glm/glm.hpp>

struct VertexAttribute
{
	glm::vec4 position;
	glm::vec3 normal;
};

class Shader
{
public:
	glm::vec3 baryCentricCoords;
	glm::vec3 baseColor { 0, 0, 0 };
	glm::mat4 modelViewprojection;
	glm::vec3 lightDir { 0, 0, 1 };
	virtual void Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex) = 0;
	virtual bool Fragment(glm::vec4& gl_FragColor, float intensity) const = 0;
};


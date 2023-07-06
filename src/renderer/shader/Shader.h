#pragma once
#include <glm/glm.hpp>

class Shader
{
public:
	glm::vec3 baseColor { 0, 0, 0 };
	virtual void Vertex(glm::vec4& gl_Position, const glm::vec4& vertexPosition, const glm::mat4& modelViewProjection) const = 0;
	virtual bool Fragment(glm::vec4& gl_FragColor, float intensity) const = 0;
};


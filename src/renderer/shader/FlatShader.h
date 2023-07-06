#pragma once
#include "Shader.h"

class FlatShader : public Shader
{
public:
	virtual void Vertex(glm::vec4& gl_Position, const glm::vec4& vertexPosition, const glm::mat4& modelViewProjection) const override;
	virtual bool Fragment(glm::vec4& gl_FragColor, float intensity) const override;
};
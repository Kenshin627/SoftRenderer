#pragma once
#include "Shader.h"

class FlatShader : public Shader
{
public:
	virtual void Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex) override;
	virtual bool Fragment(glm::vec4& gl_FragColor, float intensity) override;
};
#pragma once
#include "Shader.h"

class FlatShader : public Shader
{
public:
	virtual void Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex) override;
	virtual bool Fragment(glm::vec4& gl_FragColor) override;
};
class BarycentricShader : public FlatShader
{
public:
	bool Fragment(glm::vec4& color) override
	{
		color = glm::vec4(baryCentric, 1.0f);
		return false;
	}
};

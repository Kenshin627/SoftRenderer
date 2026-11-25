#pragma once
#include "Shader.h"

class NormalMapShader :public Shader
{
public:
	virtual void Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex) override;
	virtual bool Fragment(glm::vec4& gl_FragColor) override;
};
class MaterialPreviewShader : public NormalMapShader
{
public:
	explicit MaterialPreviewShader(bool emission) : showEmission(emission) {}
	bool Fragment(glm::vec4& color) override;
private:
	bool showEmission;
};

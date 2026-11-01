#pragma once
#include "Shader.h"

//tutorial toon chapter: ink outlines from a 3x3 Sobel filter over the depth buffer
//of the previous pass; strong edge response => black line, otherwise pass the color through
class ToonEdgeShader : public Shader
{
public:
	explicit ToonEdgeShader(float threshold = 0.02f) : edgeThreshold(threshold) {}
	virtual void Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex) override;
	virtual bool Fragment(glm::vec4& gl_fragColor) override;
private:
	float edgeThreshold;
};

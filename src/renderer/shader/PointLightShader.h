#pragma once
#include "Shader.h"

class PointLightShader : public Shader
{
public:
	virtual void Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex) override;
	virtual bool Fragment(glm::vec4& gl_FragColor) override;
public:
	glm::vec3 normals[3];
	glm::vec2 uvs[3];
	glm::vec3 positions[3];
};
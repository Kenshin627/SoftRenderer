#pragma once
#include "Shader.h"

class BlinnPhongShader : public Shader
{
public:
	virtual void Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex) override;
	virtual bool Fragment(glm::vec4& gl_FragColor, float intensity) override;
public:
	glm::vec3 normals[3];
	glm::vec2 uvs[3];
	glm::vec3 Postions[3];
};
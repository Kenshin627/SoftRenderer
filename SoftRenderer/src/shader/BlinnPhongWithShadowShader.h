#pragma once
#include "Shader.h"

class BlinnPhongWithShadowShader :public Shader
{
public:
	explicit BlinnPhongWithShadowShader(bool phong = false) : reflectionPhong(phong) {}
	virtual void Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex) override;
	virtual bool Fragment(glm::vec4& gl_FragColor) override;
private:
	bool reflectionPhong;
};
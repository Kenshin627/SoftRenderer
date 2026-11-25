#pragma once
#include "Shader.h"

class BlinnPhongWithShadowShader :public Shader
{
public:
	explicit BlinnPhongWithShadowShader(bool phong = false) : reflectionPhong(phong) {}
	virtual void Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex) override;
	virtual bool Fragment(glm::vec4& gl_FragColor) override;
private:
	float ShadowFactor(const glm::vec3& worldPos) const;
	glm::vec2 shadowDepthGradient{0.0f};
	bool reflectionPhong;
};
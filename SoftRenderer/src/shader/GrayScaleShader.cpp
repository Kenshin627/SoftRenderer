#include "GrayScaleShader.h"

GrayScaleShader::GrayScaleShader() {}

void GrayScaleShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex) { }

bool GrayScaleShader::Fragment(glm::vec4& gl_FragColor)
{
	TGAColor samplerColor = prePassColorAttachment.get(currentPixel.x, currentPixel.y);
	glm::vec3 color = { samplerColor.r / 255.0f, samplerColor.g / 255.0f, samplerColor.b / 255.0f };
	float GrayScale = 0.299f * color.r + 0.587f * color.g + 0.114f * color.b;
	gl_FragColor = glm::vec4(GrayScale, GrayScale, GrayScale, 1.0f);
	return false;
}

void GrayScaleShader::SetCurentPixel(const glm::vec2& p)
{
	currentPixel = p;
}
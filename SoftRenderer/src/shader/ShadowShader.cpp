#include "ShadowShader.h"

void ShadowShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, uint32_t vertexIndex)
{
	gl_Position = viewProjection * model * glm::vec4(vertex.position, 1.0);
}
bool ShadowShader::Fragment(glm::vec4& gl_fragColor)
{
	gl_fragColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // depth pass needs no color; set it to avoid reading an uninitialized value
	return false;
}
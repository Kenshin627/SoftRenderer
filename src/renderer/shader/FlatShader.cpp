#include "FlatShader.h"

void FlatShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	gl_Position = modelViewprojection * vertex.position;
}

bool FlatShader::Fragment(glm::vec4& gl_FragColor, float intensity)
{
	if (intensity <= 0)
	{
		return true;
	}
	gl_FragColor = { baseColor.r * intensity, baseColor.g * intensity, baseColor.b * intensity, 255 };
	return false;
}
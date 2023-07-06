#include "FlatShader.h"

void FlatShader::Vertex(glm::vec4& gl_Position, const glm::vec4& vertexPosition, const glm::mat4& modelViewProjection) const
{
	gl_Position = modelViewProjection * vertexPosition;
}

bool FlatShader::Fragment(glm::vec4& gl_FragColor, float intensity) const
{
	if (intensity <= 0)
	{
		return false;
	}
	gl_FragColor = { baseColor.r * intensity, baseColor.g * intensity, baseColor.b * intensity, 255 };
	return true;
}
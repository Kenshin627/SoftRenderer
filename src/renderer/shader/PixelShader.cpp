#include "PixelShader.h"

void PixelShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	normals[gl_VertexIndex] = vertex.normal;
	gl_Position = modelViewprojection * vertex.position;
}

//return discard
bool PixelShader::Fragment(glm::vec4& gl_FragColor)
{
	//���������ֵ���㵱ǰ���ص㷨�ߣ������ؼ������
	glm::vec3 n = normals[0] * baryCentricCoords.x + normals[1] * baryCentricCoords.y + normals[2] * baryCentricCoords.z;
	n = glm::normalize(n);
	float pixelIntensity = glm::max<float>(0.0, glm::dot(n, dLight.Direction()));
	gl_FragColor = { baseColor.r * pixelIntensity, baseColor.g * pixelIntensity, baseColor.b * pixelIntensity, 255 };
	return false;
}
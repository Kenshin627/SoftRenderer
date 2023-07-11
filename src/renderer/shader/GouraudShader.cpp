#include "GouraudShader.h"

void GouraudShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	//逐顶点计算光照，将光照计算结果在fragment重心坐标插值
	vertexIntensity[gl_VertexIndex] = glm::max<float>(0.0, glm::dot(vertex.normal, dLight.Direction()));
	gl_Position = viewprojectionMatrix * vertex.position;
}

//return discard
bool GouraudShader::Fragment(glm::vec4& gl_FragColor)
{
	//重心坐标插值计算当前像素点光照强度
	float pixelIntensity = glm::max<float>(0.0, glm::dot(vertexIntensity, baryCentricCoords));
	gl_FragColor = { baseColor.r * pixelIntensity, baseColor.g * pixelIntensity, baseColor.b * pixelIntensity, 255 };
	return false;
}
#include "ToonShader.h"

void ToonShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	//逐顶点计算光照，将光照计算结果在fragment重心坐标插值
	lightDir = glm::normalize(lightDir);
	vertexIntensity[gl_VertexIndex] = glm::max<float>(0.0, glm::dot(vertex.normal, lightDir));
	gl_Position = modelViewprojection * vertex.position;
}

//return discard
bool ToonShader::Fragment(glm::vec4& gl_FragColor, float intensity) const
{
	//重心坐标插值计算当前像素点光照强度
	float pixelIntensity = glm::max<float>(0.0, glm::dot(vertexIntensity, baryCentricCoords));
	if (pixelIntensity >= 0.8)
	{
		pixelIntensity = 1.0;
	}
	else if (pixelIntensity >= 0.6)
	{
		pixelIntensity = 0.8;
	}
	else if (pixelIntensity >= 0.4)
	{
		pixelIntensity = 0.6;
	}
	else if (pixelIntensity >= 0.2)
	{
		pixelIntensity = 0.4;
	}
	else
	{
		pixelIntensity = 0.1;
	}
	gl_FragColor = { baseColor.r * pixelIntensity, baseColor.g * pixelIntensity, baseColor.b * pixelIntensity, 255 };
	return false;
}
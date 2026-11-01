#include "ToonShader.h"

void ToonShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	//逐顶点计算光照，将光照计算结果在fragment重心坐标插值
	glm::vec3 lightDir = glm::normalize(-light->Direction());
	vertexIntensity[gl_VertexIndex] = glm::max<float>(0.0, glm::dot(glm::normalize(itModel * vertex.normal), lightDir));
	gl_Position = viewProjection * model * glm::vec4(vertex.position, 1.0f);
}

//return discard
bool ToonShader::Fragment(glm::vec4& gl_FragColor)
{
	//重心坐标插值计算当前像素点光照强度
	float pixelIntensity = glm::max<float>(0.0, glm::dot(vertexIntensity, baryCentric));
	//tutorial toon chapter quantization: three hard bands on a solid color (no texture)
	if (pixelIntensity > 0.66f)
	{
		pixelIntensity = 1.0f;
	}
	else if (pixelIntensity > 0.33f)
	{
		pixelIntensity = 0.66f;
	}
	else
	{
		pixelIntensity = 0.33f;
	}
	gl_FragColor = { baseColor.r * pixelIntensity, baseColor.g * pixelIntensity, baseColor.b * pixelIntensity, 1.0f };
	return false;
}
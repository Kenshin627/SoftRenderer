#include "PixelShader.h"

void PixelShader::Vertex(glm::vec4& gl_Position, const VertexAttribute& vertex, unsigned gl_VertexIndex)
{
	normals[gl_VertexIndex] = glm::normalize(invertTransposeModelMatrix * vertex.normal);
	gl_Position = viewprojectionMatrix * modelMatrix * vertex.position;
}

//return discard
bool PixelShader::Fragment(glm::vec4& gl_FragColor)
{
	//重心坐标插值计算当前像素点法线，逐像素计算光照
	glm::vec3 lightDir = glm::normalize(-dLight.Direction());
	glm::vec3 n = normals[0] * baryCentricCoords.x + normals[1] * baryCentricCoords.y + normals[2] * baryCentricCoords.z;
	n = glm::normalize(n);
	float pixelIntensity = glm::max<float>(0.0, glm::dot(n, lightDir));
	gl_FragColor = { baseColor.r * pixelIntensity, baseColor.g * pixelIntensity, baseColor.b * pixelIntensity, 255 };
	return false;
}